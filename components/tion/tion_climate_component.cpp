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

void TionClimateComponentBase::call_setup() {
  TionComponent::call_setup();
  this->setup_presets();
}

void TionClimateComponentBase::dump_settings(const char *TAG, const char *component) const {
  LOG_CLIMATE(component, "", this);
  LOG_UPDATE_INTERVAL(this);
#ifdef USE_TION_VERSION
  LOG_TEXT_SENSOR("  ", "Version", this->version_);
#endif
#ifdef USE_TION_BUZZER
  LOG_SWITCH("  ", "Buzzer", this->buzzer_);
#endif
#ifdef USE_TION_LED
  LOG_SWITCH("  ", "Led", this->led_);
#endif
#ifdef USE_TION_OUTDOOR_TEMPERATURE
  LOG_SENSOR("  ", "Outdoor Temperature", this->outdoor_temperature_);
#endif
#ifdef USE_TION_HEATER_POWER
  LOG_SENSOR("  ", "Heater Power", this->heater_power_);
#endif
#ifdef USE_TION_PRODUCTIVITY
  LOG_SENSOR("  ", "Productivity", this->productivity_);
#endif
#ifdef USE_TION_AIRFLOW_COUNTER
  LOG_SENSOR("  ", "Airflow Counter", this->airflow_counter_);
#endif
#ifdef USE_TION_FILTER_TIME_LEFT
  LOG_SENSOR("  ", "Filter Time Left", this->filter_time_left_);
#endif
#ifdef USE_TION_FILTER_WARNOUT
  LOG_BINARY_SENSOR("  ", "Filter Warnout", this->filter_warnout_);
#endif
#ifdef USE_TION_RESET_FILTER
  LOG_BUTTON("  ", "Reset Filter", this->reset_filter_);
  LOG_SWITCH("  ", "Reset Filter Confirm", this->reset_filter_confirm_);
#endif
#ifdef USE_TION_STATE_WARNOUT
  LOG_BINARY_SENSOR("  ", "State Warnout", this->state_warnout_);
  ESP_LOGCONFIG(TAG, "  State timeout: %.1fs", this->state_timeout_ / 1000.0f);
#endif
  ESP_LOGCONFIG(TAG, "  Batch timeout: %.1fs", this->batch_timeout_ / 1000.0f);
}

climate::ClimateTraits TionClimateComponentBase::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_visual_min_temperature(TION_MIN_TEMPERATURE);
  traits.set_visual_max_temperature(TION_MAX_TEMPERATURE);
  traits.set_visual_temperature_step(1.0f);
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
#ifdef TION_ENABLE_CLIMATE_MODE_HEAT_COOL
      climate::CLIMATE_MODE_HEAT_COOL,
#endif
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_FAN_ONLY,
  });
  for (uint8_t i = 1, max = i + TION_MAX_FAN_SPEED; i < max; i++) {
    traits.add_supported_custom_fan_mode(fan_speed_to_mode(i));
  }
  this->add_presets(traits);
  traits.set_supports_action(true);
  return traits;
}

void TionClimateComponentBase::control(const climate::ClimateCall &call) {
#ifdef TION_ENABLE_PRESETS
  if (call.get_preset().has_value()) {
    if (this->enable_preset_(*call.get_preset())) {
      return;
    }
  }
#endif

  climate::ClimateMode mode = this->mode;
  if (call.get_mode().has_value()) {
    mode = *call.get_mode();
    ESP_LOGD(TAG, "Set mode %s", LOG_STR_ARG(climate::climate_mode_to_string(mode)));
    this->preset = climate::CLIMATE_PRESET_NONE;
  }

  uint8_t fan_speed = fan_mode_to_speed(this->custom_fan_mode);
  if (call.get_custom_fan_mode().has_value()) {
    fan_speed = fan_mode_to_speed(*call.get_custom_fan_mode());
    ESP_LOGD(TAG, "Set fan speed %u", fan_speed);
    this->preset = climate::CLIMATE_PRESET_NONE;
  }

  float target_temperature = this->target_temperature;
  if (call.get_target_temperature().has_value()) {
    target_temperature = *call.get_target_temperature();
    ESP_LOGD(TAG, "Set target temperature %d °C", int(target_temperature));
    this->preset = climate::CLIMATE_PRESET_NONE;
  }

  this->control_climate_state(mode, fan_speed, target_temperature, TION_CLIMATE_GATE_POSITION_NONE);
}

void TionClimateComponentBase::set_fan_speed_(uint8_t fan_speed) {
  if (fan_speed > 0 && fan_speed <= TION_MAX_FAN_SPEED) {
    this->custom_fan_mode = fan_speed_to_mode(fan_speed);
  } else {
    auto ok_fan_speed = this->mode == climate::CLIMATE_MODE_OFF && fan_speed == 0;
    if (!ok_fan_speed) {
      ESP_LOGW(TAG, "Unsupported fan speed %u (max: %u)", fan_speed, TION_MAX_FAN_SPEED);
    }
  }
}

}  // namespace tion
}  // namespace esphome
