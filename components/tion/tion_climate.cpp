#include "esphome/core/log.h"

#include "tion_climate.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_climate";

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
#ifdef TION_ENABLE_PRESETS
  if (!this->supported_presets_.empty()) {
    traits.set_supported_presets(this->supported_presets_);
    traits.add_supported_preset(climate::CLIMATE_PRESET_NONE);
  }
#endif
  return traits;
}

void TionClimate::control(const climate::ClimateCall &call) {
#ifdef TION_ENABLE_PRESETS
  bool preset_set = false;
  if (call.get_preset().has_value()) {
    const auto new_preset = *call.get_preset();
    const auto old_preset = *this->preset;
    if (new_preset != old_preset) {
      this->cancel_preset_(old_preset);
      preset_set = this->enable_preset_(new_preset);
    }
  }
#endif

  if (call.get_mode().has_value()) {
#ifdef TION_ENABLE_PRESETS
    if (preset_set) {
      ESP_LOGW(TAG, "%s preset enabled. Ignore change mode.",
               LOG_STR_ARG(climate::climate_preset_to_string(*this->preset)));
    } else
#endif
    {
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
#ifdef TION_ENABLE_PRESETS
    if (preset_set) {
      ESP_LOGW(TAG, "%s preset enabled. Ignore change fan speed.",
               LOG_STR_ARG(climate::climate_preset_to_string(*this->preset)));
    } else
#endif
    {
      this->set_fan_speed_(this->fan_mode_to_speed_(call.get_custom_fan_mode()));
      this->preset = climate::CLIMATE_PRESET_NONE;
    }
  }

  if (call.get_target_temperature().has_value()) {
#ifdef TION_ENABLE_PRESETS
    if (preset_set) {
      ESP_LOGW(TAG, "%s preset enabled. Ignore change target temperature.",
               LOG_STR_ARG(climate::climate_preset_to_string(*this->preset)));
    } else
#endif
    {
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

#ifdef TION_ENABLE_PRESETS
bool TionClimate::enable_preset_(climate::ClimatePreset preset) {
  ESP_LOGD(TAG, "Enable preset %s", LOG_STR_ARG(climate::climate_preset_to_string(preset)));
  if (preset == climate::CLIMATE_PRESET_BOOST) {
    if (!this->enable_boost_()) {
      ESP_LOGW(TAG, "Boost time is not configured");
      return false;
    }
    this->saved_preset_ = *this->preset;
  }

  if (*this->preset == climate::CLIMATE_PRESET_NONE) {
    this->presets_[climate::CLIMATE_PRESET_NONE].mode = this->mode;
    this->presets_[climate::CLIMATE_PRESET_NONE].fan_speed = this->get_fan_speed_();
    this->presets_[climate::CLIMATE_PRESET_NONE].target_temperature = this->target_temperature;
  }

  // дополнительно проверим, что пресет был предварительно сохранен
  // в противном случае можем получить зимой отстутсвие подогрева в детской
  if (this->presets_[preset].fan_speed != 0) {
    this->mode = this->presets_[preset].mode;
    this->set_fan_speed_(this->presets_[preset].fan_speed);
    this->target_temperature = this->presets_[preset].target_temperature;
  }
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
#endif

}  // namespace tion
}  // namespace esphome
