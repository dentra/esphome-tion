#include <cstddef>

#include "esphome/core/log.h"
#include "esphome/core/application.h"

#include "tion.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion";

// boost time update interval
#define BOOST_TIME_UPDATE_INTERVAL_SEC 20

// application scheduler name
static const char *const ASH_BOOST = "tion-boost";

climate::ClimateTraits TionClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_visual_min_temperature(0.0);
  traits.set_visual_max_temperature(25.0);
  traits.set_visual_temperature_step(1.0);
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_FAN_ONLY,
  });
  for (uint8_t i = 1, max = i + this->max_fan_speed_; i < max; i++) {
    traits.add_supported_custom_fan_mode(this->fan_speed_to_mode_(i));
  }
  if (!this->supported_presets_.empty()) {
    traits.set_supported_presets(this->supported_presets_);
    traits.add_supported_preset(climate::CLIMATE_PRESET_NONE);
  }
  return traits;
}

void TionClimate::control(const climate::ClimateCall &call) {
  bool preset_set = false;
  if (call.get_preset().has_value()) {
    const auto new_preset = *call.get_preset();
    const auto old_preset = *this->preset;
    if (new_preset != old_preset) {
      this->cancel_preset_(old_preset);
      preset_set = this->enable_preset_(new_preset);
    }
  }

  if (call.get_mode().has_value()) {
    if (preset_set) {
      ESP_LOGW(TAG, "%s preset enabled. Ignore change mode.",
               LOG_STR_ARG(climate::climate_preset_to_string(*this->preset)));
    } else {
      auto mode = *call.get_mode();
      switch (mode) {
        case climate::CLIMATE_MODE_OFF:
        case climate::CLIMATE_MODE_HEAT:
        case climate::CLIMATE_MODE_FAN_ONLY:
          this->mode = mode;
          ESP_LOGD(TAG, "Set mode %u", mode);
          break;
        default:
          ESP_LOGW(TAG, "Unsupported mode %d", mode);
          return;
      }
    }
  }

  if (call.get_custom_fan_mode().has_value()) {
    if (preset_set) {
      ESP_LOGW(TAG, "%s preset enabled. Ignore change fan speed.",
               LOG_STR_ARG(climate::climate_preset_to_string(*this->preset)));
    } else {
      this->set_fan_speed_(this->fan_mode_to_speed_(call.get_custom_fan_mode()));
      this->preset = climate::CLIMATE_PRESET_NONE;
    }
  }

  if (call.get_target_temperature().has_value()) {
    if (preset_set) {
      ESP_LOGW(TAG, "%s preset enabled. Ignore change target temperature.",
               LOG_STR_ARG(climate::climate_preset_to_string(*this->preset)));
    } else {
      ESP_LOGD(TAG, "Set target temperature %f", target_temperature);
      const auto target_temperature = *call.get_target_temperature();
      this->target_temperature = target_temperature;
      this->preset = climate::CLIMATE_PRESET_NONE;
    }
  }

  this->write_climate_state();
}

void TionClimate::set_fan_speed_(uint8_t fan_speed) {
  if (fan_speed > 0 && fan_speed <= this->max_fan_speed_) {
    this->custom_fan_mode = this->fan_speed_to_mode_(fan_speed);
  } else {
    if (!(this->mode == climate::CLIMATE_MODE_OFF && fan_speed == 0)) {
      ESP_LOGW(TAG, "Unsupported fan speed %u (max: %u)", fan_speed, this->max_fan_speed_);
    }
  }
}

bool TionClimate::enable_preset_(climate::ClimatePreset preset) {
  ESP_LOGD(TAG, "Enable preset %s", LOG_STR_ARG(climate::climate_preset_to_string(preset)));
  if (preset == climate::CLIMATE_PRESET_BOOST) {
    if (!this->enable_boost_()) {
      return false;
    }
    this->saved_preset_ = *this->preset;
  }

  if (*this->preset == climate::CLIMATE_PRESET_NONE) {
    this->presets_[climate::CLIMATE_PRESET_NONE].mode = this->mode;
    this->presets_[climate::CLIMATE_PRESET_NONE].fan_speed = this->get_fan_speed_();
    this->presets_[climate::CLIMATE_PRESET_NONE].target_temperature = this->target_temperature;
  }

  this->mode = this->presets_[preset].mode;
  this->set_fan_speed_(this->presets_[preset].fan_speed);
  this->target_temperature = this->presets_[preset].target_temperature;
  this->preset = preset;

  return true;
}

void TionClimate::cancel_preset_(climate::ClimatePreset preset) {
  ESP_LOGD(TAG, "Cancel preset %s", LOG_STR_ARG(climate::climate_preset_to_string(preset)));
  if (preset == climate::CLIMATE_PRESET_BOOST) {
    this->cancel_boost_();
    this->enable_preset_(this->saved_preset_);
  }
}

void TionComponent::setup() {
  if (this->boost_time_ && std::isnan(this->boost_time_->state)) {
    auto call = this->boost_time_->make_call();
    call.set_value(DEFAULT_BOOST_TIME_SEC / 60);
    call.perform();
  }
}

void TionComponent::update_dev_status_(const dentra::tion::tion_dev_status_t &status) {
  if (this->version_ != nullptr) {
    this->version_->publish_state(str_snprintf("%04X", 4, status.firmware_version));
  }
  ESP_LOGV(TAG, "Work Mode       : %02X", status.work_mode);
  ESP_LOGV(TAG, "Device type     : %04X", status.device_type);
  ESP_LOGV(TAG, "Device sub-type : %04X", status.device_subtype);
  ESP_LOGV(TAG, "Hardware version: %04X", status.hardware_version);
  ESP_LOGV(TAG, "Firmware version: %04X", status.firmware_version);
}

bool TionClimateComponentBase::enable_boost_() {
  uint32_t boost_time = this->boost_time_ ? this->boost_time_->state * 60 : DEFAULT_BOOST_TIME_SEC;
  if (boost_time == 0) {
    ESP_LOGW(TAG, "Boost time is not configured");
    return false;
  }

  // if boost_time_left not configured, just schedule stop boost after boost_time
  if (this->boost_time_left_ == nullptr) {
    ESP_LOGD(TAG, "Schedule boost timeout for %u s", boost_time);
    App.scheduler.set_timeout(this, ASH_BOOST, boost_time * 1000, [this]() {
      this->cancel_preset_(*this->preset);
      this->write_climate_state();
    });
    return true;
  }

  // if boost_time_left is configured, schedule update it
  ESP_LOGD(TAG, "Schedule boost interval up to %u s", boost_time);
  App.scheduler.set_interval(this, ASH_BOOST, BOOST_TIME_UPDATE_INTERVAL_SEC * 1000, [this, boost_time]() {
    int time_left;
    if (std::isnan(this->boost_time_left_->state)) {
      time_left = boost_time;
    } else {
      time_left = (this->boost_time_left_->state * (boost_time / 100)) - BOOST_TIME_UPDATE_INTERVAL_SEC;
    }
    ESP_LOGV(TAG, "Boost time left %d s", time_left);
    if (time_left > 0) {
      this->boost_time_left_->publish_state(static_cast<float>(time_left) / static_cast<float>(boost_time / 100));
    } else {
      this->cancel_preset_(*this->preset);
      this->write_climate_state();
    }
  });

  return true;
}

void TionClimateComponentBase::cancel_boost_() {
  if (this->boost_time_left_) {
    ESP_LOGV(TAG, "Cancel boost interval");
    App.scheduler.cancel_interval(this, ASH_BOOST);
  } else {
    ESP_LOGV(TAG, "Cancel boost timeout");
    App.scheduler.cancel_timeout(this, ASH_BOOST);
  }
  this->boost_time_left_->publish_state(NAN);
}

void TionClimateComponentBase::dump_component_config(const char *TAG, const char *component) const {
  LOG_CLIMATE(component, "", this);
  LOG_TEXT_SENSOR("  ", "Version", this->version_);
  LOG_SWITCH("  ", "Buzzer", this->buzzer_);
  LOG_SWITCH("  ", "Led", this->led_);
  LOG_SENSOR("  ", "Outdoor Temperature", this->outdoor_temperature_);
  LOG_SENSOR("  ", "Heater Power", this->heater_power_);
  LOG_SENSOR("  ", "Airflow Counter", this->airflow_counter_);
  LOG_SENSOR("  ", "Filter Time Left", this->filter_time_left_);
  LOG_BINARY_SENSOR("  ", "Filter Warnout", this->filter_warnout_);
  LOG_NUMBER("  ", "Boost Time", this->boost_time_);
  LOG_SENSOR("  ", "Boost Time Left", this->boost_time_left_);
}

}  // namespace tion
}  // namespace esphome
