#include <cinttypes>
#include <cstdint>
#include <cmath>
#include <string>
#include <cstring>

#include "esphome/core/log.h"
#include "esphome/core/preferences.h"
#include "esphome/core/helpers.h"
#include "esphome/components/climate/climate_mode.h"

#include "tion_component.h"
#include "tion_climate.h"
#include "tion_climate_component.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_climate_component";

// boost time update interval
#define BOOST_TIME_UPDATE_INTERVAL_SEC 20

// application scheduler name
static const char *const ASH_BOOST = "tion-boost";

void TionClimateComponentBase::call_setup() {
  TionComponent::call_setup();

#ifdef TION_ENABLE_PRESETS_WITH_API
  this->presets_rtc_ = global_preferences->make_preference<TionPreset[TION_MAX_PRESETS]>(fnv1_hash("presets"));
  if (this->presets_rtc_.load(&this->presets_)) {
    this->presets_[0].fan_speed = 0;  // reset initialization
    ESP_LOGD(TAG, "Presets loaded");
  }
  this->register_service(&TionClimateComponentBase::update_preset_service_, "update_preset",
                         {"preset", "mode", "fan_speed", "target_temperature", "gate_position"});
#endif  // TION_ENABLE_PRESETS_WITH_API
}

#ifdef TION_ENABLE_PRESETS
#ifdef USE_API
// esphome services allows only pass copy of strings
void TionClimateComponentBase::update_preset_service_(std::string preset_str, std::string mode_str, int32_t fan_speed,
                                                      int32_t target_temperature, std::string gate_position_str) {
  climate::ClimatePreset preset = climate::ClimatePreset::CLIMATE_PRESET_NONE;
  if (preset_str.length() > 0) {
    auto preset_ch = std::tolower(preset_str[0]);
    if (preset_ch == 'h') {  // Home
      preset = climate::ClimatePreset::CLIMATE_PRESET_HOME;
    } else if (preset_ch == 'a') {  // Away/Activity
      if (preset_str.length() > 1) {
        preset_ch = std::tolower(preset_str[1]);
        if (preset_ch == 'w') {  // aWay
          preset = climate::ClimatePreset::CLIMATE_PRESET_AWAY;
        } else if (preset_ch == 'c') {  // aCtivity
          preset = climate::ClimatePreset::CLIMATE_PRESET_ACTIVITY;
        }
      }
    } else if (preset_ch == 'b') {  // Boost
      preset = climate::ClimatePreset::CLIMATE_PRESET_BOOST;
    } else if (preset_ch == 'c') {  // Comform
      preset = climate::ClimatePreset::CLIMATE_PRESET_COMFORT;
    } else if (preset_ch == 'e') {  // Eco
      preset = climate::ClimatePreset::CLIMATE_PRESET_ECO;
    } else if (preset_ch == 's') {  // Sleep
      preset = climate::ClimatePreset::CLIMATE_PRESET_SLEEP;
    }
  }

  auto mode = climate::ClimateMode::CLIMATE_MODE_AUTO;
  if (mode_str.length() > 0) {
    auto mode_ch = std::tolower(mode_str[0]);
    if (mode_ch == 'h') {  // Heat
      mode = climate::ClimateMode::CLIMATE_MODE_HEAT;
    } else if (mode_ch == 'f') {  // Fan_only
      mode = climate::ClimateMode::CLIMATE_MODE_FAN_ONLY;
    } else if (mode_ch == 'o') {  // Off
      mode = climate::ClimateMode::CLIMATE_MODE_OFF;
    }
  }

  TionClimateGatePosition gate_position = TION_CLIMATE_GATE_POSITION_NONE;
  if (gate_position_str.length() > 0) {
    auto gate_position_ch = std::tolower(gate_position_str[0]);
    if (gate_position_ch == 'o') {  // Outdoor
      gate_position = TION_CLIMATE_GATE_POSITION_OUTDOOR;
    } else if (gate_position_ch == 'i') {  // Indoor
      gate_position = TION_CLIMATE_GATE_POSITION_INDOOR;
    } else if (gate_position_ch == 'm') {  // Mixed
      gate_position = TION_CLIMATE_GATE_POSITION_MIXED;
    }
  }

  if (this->update_preset(preset, mode, fan_speed, target_temperature, gate_position)) {
    if (this->presets_rtc_.save(&this->presets_)) {
      ESP_LOGCONFIG(TAG, "Preset was updated:");
      this->dump_preset_(TAG, preset);
    }
  } else {
    ESP_LOGW(TAG, "Preset %s was't updated", preset_str.c_str());
  }
}
#endif  // USE_API

bool TionClimateComponentBase::enable_boost() {
  auto boost_time = this->get_boost_time_();
  if (boost_time == 0) {
    ESP_LOGW(TAG, "Boost time is not configured");
    return false;
  }

  // if boost_time_left not configured, just schedule stop boost after boost_time
  if (this->boost_time_left_ == nullptr) {
    ESP_LOGD(TAG, "Schedule boost timeout for %" PRIu32 " s", boost_time);
    this->set_timeout(ASH_BOOST, boost_time * 1000, [this]() { this->cancel_preset_(climate::CLIMATE_PRESET_BOOST); });
    return true;
  }

  // if boost_time_left is configured, schedule update it
  ESP_LOGD(TAG, "Schedule boost interval up to %" PRIu32 " s", boost_time);
  this->boost_time_left_->publish_state(static_cast<float>(boost_time));
  this->set_interval(ASH_BOOST, BOOST_TIME_UPDATE_INTERVAL_SEC * 1000, [this]() {
    const int32_t time_left = static_cast<int32_t>(this->boost_time_left_->state) - BOOST_TIME_UPDATE_INTERVAL_SEC;
    ESP_LOGV(TAG, "Boost time left %" PRId32 " s", time_left);
    if (time_left > 0) {
      this->boost_time_left_->publish_state(static_cast<float>(time_left));
    } else {
      this->cancel_preset_(climate::CLIMATE_PRESET_BOOST);
    }
  });

  return true;
}

void TionClimateComponentBase::cancel_boost() {
  if (this->boost_time_left_) {
    ESP_LOGV(TAG, "Cancel boost interval");
    this->cancel_interval(ASH_BOOST);
    this->boost_time_left_->publish_state(NAN);
  } else {
    ESP_LOGV(TAG, "Cancel boost timeout");
    this->cancel_timeout(ASH_BOOST);
  }
}
#endif  // TION_ENABLE_PRESETS

void TionClimateComponentBase::dump_settings(const char *TAG, const char *component) const {
  LOG_CLIMATE(component, "", this);
  LOG_UPDATE_INTERVAL(this);
  LOG_TEXT_SENSOR("  ", "Version", this->version_);
  LOG_SWITCH("  ", "Buzzer", this->buzzer_);
  LOG_SWITCH("  ", "Led", this->led_);
  LOG_SENSOR("  ", "Outdoor Temperature", this->outdoor_temperature_);
  LOG_SENSOR("  ", "Heater Power", this->heater_power_);
  LOG_SENSOR("  ", "Productivity", this->productivity_);
  LOG_SENSOR("  ", "Airflow Counter", this->airflow_counter_);
  LOG_SENSOR("  ", "Filter Time Left", this->filter_time_left_);
  LOG_BINARY_SENSOR("  ", "Filter Warnout", this->filter_warnout_);
  LOG_BUTTON("  ", "Reset Filter", this->reset_filter_);
  LOG_SWITCH("  ", "Reset Filter Confirm", this->reset_filter_confirm_);
  LOG_BINARY_SENSOR("  ", "State Warnout", this->filter_warnout_);
  ESP_LOGCONFIG(TAG, "  State timeout: %.1fs", this->state_timeout_ / 1000.0f);
  ESP_LOGCONFIG(TAG, "  Batch timeout: %.1fs", this->batch_timeout_ / 1000.0f);
#ifdef TION_ENABLE_PRESETS
  LOG_NUMBER("  ", "Boost Time", this->boost_time_);
  LOG_SENSOR("  ", "Boost Time Left", this->boost_time_left_);
#endif  // TION_ENABLE_PRESETS
}

}  // namespace tion
}  // namespace esphome
