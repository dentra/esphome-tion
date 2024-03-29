#include "esphome/core/log.h"

#include "tion_climate_helpers.h"
#include "tion_climate.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_climate";

int find_climate_preset(const std::string &preset) {
#ifndef USE_ARDUINO
  const auto preset_upper = str_upper_case(preset);
  for (uint8_t i = climate::CLIMATE_PRESET_NONE; i <= climate::CLIMATE_PRESET_ACTIVITY; i++) {
    const auto preset_climate_index = static_cast<climate::ClimatePreset>(i);
    const auto preset_climate = LOG_STR_ARG(climate::climate_preset_to_string(preset_climate_index));
    if (preset_upper == preset_climate) {
      return preset_climate_index;
    }
  }
#endif
  return -1;
}

climate::ClimateTraits TionClimate2::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_visual_min_temperature(this->parent_->api()->traits().min_target_temperature);
  traits.set_visual_max_temperature(this->parent_->api()->traits().max_target_temperature);
  traits.set_visual_temperature_step(1.0f);
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
#ifdef TION_ENABLE_CLIMATE_MODE_HEAT_COOL
      climate::CLIMATE_MODE_HEAT_COOL,
#endif
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_FAN_ONLY,
  });
  for (uint8_t i = 1, max = i + this->parent_->api()->traits().max_fan_speed; i < max; i++) {
    traits.add_supported_custom_fan_mode(fan_speed_to_mode(i));
  }
  for (auto &&preset : this->parent_->api()->get_presets()) {
    const auto preset_index = find_climate_preset(preset);
    if (preset_index < 0) {
      traits.add_supported_custom_preset(preset);
    } else {
      traits.add_supported_preset(static_cast<climate::ClimatePreset>(preset_index));
    }
  }
  traits.set_supports_action(true);
  return traits;
}

void TionClimate2::dump_config() {
  LOG_CLIMATE("", "Tion Climate", this);
  this->dump_traits_(TAG);
}

void TionClimate2::control(const climate::ClimateCall &call) {
  auto *tion = this->parent_->make_call();
  if (tion == nullptr) {
    return;
  }

#ifndef USE_ARDUINO
  if (call.get_preset().has_value()) {
    const auto preset_climate = LOG_STR_ARG(climate::climate_preset_to_string(*call.get_preset()));
    for (auto &&preset : this->parent_->api()->get_presets()) {
      const auto preset_upper = str_upper_case(preset);
      if (preset_upper == preset_climate) {
        ESP_LOGD(TAG, "Set preset %s", preset.c_str());
        this->parent_->api()->enable_preset(preset, tion);
        break;
      }
    }
  }
#endif

  if (call.get_custom_preset().has_value()) {
    const auto &preset = *call.get_custom_preset();
    ESP_LOGD(TAG, "Set custom preset %s", preset.c_str());
    this->parent_->api()->enable_preset(preset, tion);
  }

  if (call.get_mode().has_value()) {
    const auto mode = *call.get_mode();
    ESP_LOGD(TAG, "Set mode %s", LOG_STR_ARG(climate::climate_mode_to_string(mode)));
    if (mode == climate::CLIMATE_MODE_OFF) {
      tion->set_power_state(false);
    } else if (mode == climate::CLIMATE_MODE_HEAT_COOL) {
      tion->set_power_state(true);
    } else {
      tion->set_power_state(true);
      tion->set_heater_state(mode == climate::CLIMATE_MODE_HEAT);
    }
  }

  if (call.get_custom_fan_mode().has_value()) {
    const auto fan_speed = fan_mode_to_speed(*call.get_custom_fan_mode());
    ESP_LOGD(TAG, "Set fan speed %u", fan_speed);
    tion->set_fan_speed(fan_speed);
  }

  if (call.get_target_temperature().has_value()) {
    const int8_t target_temperature = *call.get_target_temperature();
    ESP_LOGD(TAG, "Set target temperature %d °C", target_temperature);
    tion->set_target_temperature(target_temperature);
  }

  tion->perform();
}

void TionClimate2::on_state_(const TionState &state) {
  bool has_changes = false;

  climate::ClimateMode mode;
  climate::ClimateAction action;
  if (!state.power_state) {
    mode = climate::CLIMATE_MODE_OFF;
    action = climate::CLIMATE_ACTION_OFF;
  } else if (state.heater_state) {
    mode = climate::CLIMATE_MODE_HEAT;

    action = state.is_heating(this->parent_->api()->traits())  //-//
                 ? climate::CLIMATE_ACTION_HEATING
                 : climate::CLIMATE_ACTION_FAN;
  } else {
    mode = climate::CLIMATE_MODE_FAN_ONLY;
    action = climate::CLIMATE_ACTION_OFF;
  }

  if (this->mode != mode) {
    this->mode = mode;
    has_changes = true;
  }
  if (this->action != action) {
    this->action = action;
    has_changes = true;
  }
  if (int8_t(this->current_temperature) != state.current_temperature) {
    this->current_temperature = state.current_temperature;
    has_changes = true;
  }
  if (int8_t(this->target_temperature) != state.target_temperature) {
    this->target_temperature = state.target_temperature;
    has_changes = true;
  }
  if (this->set_fan_speed_(state.fan_speed)) {
    has_changes = true;
  }

  const auto active_preset = this->parent_->api()->get_active_preset();
#ifndef USE_ARDUINO
  const auto climate_preset = find_climate_preset(active_preset);
  if (climate_preset >= 0) {
    if (this->preset.value_or(-1) != climate_preset) {
      this->preset = static_cast<climate::ClimatePreset>(climate_preset);
      this->custom_preset.reset();
      has_changes = true;
    }
  } else
#endif
      if (this->custom_preset.value_or("") != active_preset) {
    this->custom_preset = active_preset;
    this->preset.reset();
    has_changes = true;
  }

  if (this->parent_->get_force_update() || has_changes) {
    this->publish_state();
  }
}

bool TionClimate2::set_fan_speed_(uint8_t fan_speed) {
  if (fan_speed > 0 && fan_speed <= this->parent_->api()->traits().max_fan_speed) {
    if (fan_mode_to_speed(this->custom_fan_mode) != fan_speed) {
      this->custom_fan_mode = fan_speed_to_mode(fan_speed);
      return true;
    }
    return false;
  }

  auto ok_fan_speed = this->mode == climate::CLIMATE_MODE_OFF && fan_speed == 0;
  if (!ok_fan_speed) {
    ESP_LOGW(TAG, "Unsupported fan speed %u (max: %u)", fan_speed, this->parent_->api()->traits().max_fan_speed);
  }

  return true;
}

}  // namespace tion
}  // namespace esphome
