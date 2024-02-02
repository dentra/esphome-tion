#include "esphome/core/defines.h"
#ifdef USE_CLIMATE
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
#include "tion_climate_component.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_climate_component";

TionClimateComponentBase::TionClimateComponentBase(TionVPortType vport_type) : vport_type_(vport_type) {
  this->target_temperature = NAN;
#ifdef TION_ENABLE_PRESETS
  this->preset = climate::CLIMATE_PRESET_NONE;
#endif
}

void TionClimateComponentBase::call_setup() {
  TionComponent::call_setup();
#ifdef TION_ENABLE_PRESETS
  this->setup_presets();
#endif
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
#ifdef USE_TION_WORK_TIME
  LOG_SENSOR("  ", "Work Time", this->work_time_);
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
#ifdef TION_ENABLE_PRESETS
  this->add_presets(traits);
#endif
  traits.set_supports_action(true);
  return traits;
}

void TionClimateComponentBase::control(const climate::ClimateCall &call) {
  auto gate_position = TionGatePosition::NONE;
  auto mode = this->mode;
  auto fan_speed = fan_mode_to_speed(this->custom_fan_mode);
  auto target_temperature = this->target_temperature;

  const bool has_changes = call.get_mode().has_value() || call.get_custom_fan_mode().has_value() ||
                           call.get_target_temperature().has_value();

#ifdef TION_ENABLE_PRESETS
  const auto old_preset = this->preset.value_or(climate::CLIMATE_PRESET_NONE);
  if (this->has_presets()) {
    if (call.get_preset().has_value()) {
      const auto new_preset = *call.get_preset();
      auto *preset_data = this->presets_activate_preset_(new_preset, this, this);
      if (preset_data) {
        ESP_LOGD(TAG, "Set preset %s", LOG_STR_ARG(climate::climate_preset_to_string(new_preset)));
        gate_position = preset_data->gate_position;
        mode = preset_data->mode;
        fan_speed = preset_data->fan_speed;
        target_temperature = preset_data->target_temperature;
        this->preset = new_preset;
        // если буст, то больне ничеего не даем изменять и выходим
        if (new_preset == climate::CLIMATE_PRESET_BOOST) {
          if (has_changes) {
            ESP_LOGW(TAG, "No more changes can be performed");
            if (call.get_mode().has_value()) {
              ESP_LOGW(TAG, "  Change of mode was skipped");
            }
            if (call.get_custom_fan_mode().has_value()) {
              ESP_LOGW(TAG, "  Change of fan speed was skipped");
            }
            if (call.get_target_temperature().has_value()) {
              ESP_LOGW(TAG, "  Change of target temperature was skipped");
            }
          }
          this->control_climate_state(mode, fan_speed, target_temperature, gate_position);
          return;
        }
      }
    } else if (old_preset == climate::CLIMATE_PRESET_BOOST && has_changes) {
      auto *preset_data = this->presets_activate_preset_(this->presets_saved_preset_, this, this);
      if (preset_data) {
        ESP_LOGD(TAG, "Restored preset %s",
                 LOG_STR_ARG(climate::climate_preset_to_string(this->presets_saved_preset_)));
        gate_position = preset_data->gate_position;
        mode = preset_data->mode;
        fan_speed = preset_data->fan_speed;
        target_temperature = preset_data->target_temperature;
      }
    }
  }
  const auto new_preset = this->preset.value_or(climate::CLIMATE_PRESET_NONE);
#else
  const auto old_preset = climate::CLIMATE_PRESET_NONE;
  const auto new_preset = climate::CLIMATE_PRESET_NONE;
#endif

  if (call.get_mode().has_value()) {
    mode = *call.get_mode();
    ESP_LOGD(TAG, "Set mode %s", LOG_STR_ARG(climate::climate_mode_to_string(mode)));
  }

  if (call.get_custom_fan_mode().has_value()) {
    fan_speed = fan_mode_to_speed(*call.get_custom_fan_mode());
    ESP_LOGD(TAG, "Set fan speed %u", fan_speed);
  }

  if (call.get_target_temperature().has_value()) {
    target_temperature = *call.get_target_temperature();
    ESP_LOGD(TAG, "Set target temperature %d °C", int(target_temperature));
  }

  if (!has_changes && old_preset == new_preset) {
    ESP_LOGW(TAG, "No changes was performed");
    return;
  }

#ifdef TION_ENABLE_PRESETS
  // в любом случае сбрасываем пресет если были изменения
  if (has_changes) {
    if (old_preset != climate::CLIMATE_PRESET_NONE) {
      ESP_LOGD(TAG, "Reset preset");
      this->preset = climate::CLIMATE_PRESET_NONE;
    }
  }
#endif

  this->control_climate_state(mode, fan_speed, target_temperature, gate_position);
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
#endif  // USE_CLIMATE
