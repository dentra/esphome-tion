#include "esphome/core/defines.h"
#ifdef USE_CLIMATE
#include "esphome/core/log.h"
#include "tion_climate.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_climate";

climate::ClimateTraits TionClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_visual_min_temperature(1.0f);
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
    traits.add_supported_custom_fan_mode(this->fan_speed_to_mode_(i));
  }
#ifdef TION_ENABLE_PRESETS
  traits.add_supported_preset(climate::CLIMATE_PRESET_NONE);
  this->for_each_preset_([&traits](auto index) { traits.add_supported_preset(index); });
#endif
  traits.set_supports_action(true);
  return traits;
}

void TionClimate::control(const climate::ClimateCall &call) {
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

  uint8_t fan_speed = this->fan_mode_to_speed_(this->custom_fan_mode);
  if (call.get_custom_fan_mode().has_value()) {
    fan_speed = this->fan_mode_to_speed_(call.get_custom_fan_mode());
    ESP_LOGD(TAG, "Set fan speed %u", fan_speed);
    this->preset = climate::CLIMATE_PRESET_NONE;
  }

  int8_t target_temperature = this->target_temperature;
  if (call.get_target_temperature().has_value()) {
    target_temperature = *call.get_target_temperature();
    ESP_LOGD(TAG, "Set target temperature %d °C", target_temperature);
    this->preset = climate::CLIMATE_PRESET_NONE;
  }

  this->control_climate_state(mode, fan_speed, target_temperature, TION_CLIMATE_GATE_POSITION_AUTO);
}

void TionClimate::set_fan_speed_(uint8_t fan_speed) {
  if (fan_speed > 0 && fan_speed <= TION_MAX_FAN_SPEED) {
    this->custom_fan_mode = this->fan_speed_to_mode_(fan_speed);
  } else {
    auto ok_fan_speed = this->mode == climate::CLIMATE_MODE_OFF && fan_speed == 0;
    if (!ok_fan_speed) {
      ESP_LOGW(TAG, "Unsupported fan speed %u (max: %u)", fan_speed, TION_MAX_FAN_SPEED);
    }
  }
}

void TionClimate::dump_presets(const char *tag) const {
#ifdef TION_ENABLE_PRESETS
  auto has_presets = false;
  this->for_each_preset_([&has_presets](auto index) { has_presets = true; });
  if (has_presets) {
    ESP_LOGCONFIG(tag, "  Presets (fan_speed, target_temperature, mode, gate_position):");
    this->for_each_preset_([tag, this](auto index) { this->dump_preset_(tag, index); });
  }
#endif
}

#ifdef TION_ENABLE_PRESETS
void TionClimate::dump_preset_(const char *tag, climate::ClimatePreset index) const {
  auto gate_position_to_string = [](TionClimateGatePosition gp) -> const char * {
    switch (gp) {
      case TION_CLIMATE_GATE_POSITION_AUTO:
        return "auto";
      case TION_CLIMATE_GATE_POSITION_OUTDOOR:
        return "outdoor";
      case TION_CLIMATE_GATE_POSITION_INDOOR:
        return "indoor";
      case TION_CLIMATE_GATE_POSITION_MIXED:
        return "mixed";
      default:
        return "unknown";
    }
  };
  const auto &preset = this->presets_[index];
  const auto *preset_str = LOG_STR_ARG(climate::climate_preset_to_string(index));
  const auto *mode_str = LOG_STR_ARG(climate::climate_mode_to_string(preset.mode));
  const auto *gate_pos_str = gate_position_to_string(preset.gate_position);
  ESP_LOGCONFIG(tag, "    %-8s: %u, %2d, %-8s, %s", str_lower_case(preset_str).c_str(), preset.fan_speed,
                preset.target_temperature, str_lower_case(mode_str).c_str(), gate_pos_str);
}

bool TionClimate::enable_preset_(climate::ClimatePreset new_preset) {
  const auto old_preset = *this->preset;
  if (new_preset == old_preset) {
    ESP_LOGD(TAG, "Preset was not changed");
    return false;
  }

  if (old_preset == climate::CLIMATE_PRESET_BOOST) {
    ESP_LOGD(TAG, "Cancel preset boost");
    this->cancel_boost();
  }

  ESP_LOGD(TAG, "Enable preset %s", LOG_STR_ARG(climate::climate_preset_to_string(new_preset)));
  if (new_preset == climate::CLIMATE_PRESET_BOOST) {
    if (!this->enable_boost()) {
      return false;
    }
    this->saved_preset_ = old_preset;
    // инициализируем дефолный пресет NONE чтобы можно было в него восстановиться в любом случае
    if (!this->presets_[climate::CLIMATE_PRESET_NONE].is_initialized() && old_preset != climate::CLIMATE_PRESET_NONE) {
      this->update_default_preset_();
    }
  }

  // если был пресет NONE, то сохраним его текущее состояние
  if (old_preset == climate::CLIMATE_PRESET_NONE) {
    this->update_default_preset_();
  }

  // дополнительно проверим, что пресет был предварительно сохранен (см. блок выше)
  // в противном случае можем получить зимой, например, отстутсвие подогрева
  // т.е. неинициализированный пресет не активируем
  if (!this->presets_[new_preset].is_initialized()) {
    ESP_LOGW(TAG, "No data for preset %s", LOG_STR_ARG(climate::climate_preset_to_string(new_preset)));
    return false;
  }

  this->control_climate_state(this->presets_[new_preset].mode, this->presets_[new_preset].fan_speed,
                              this->presets_[new_preset].target_temperature, this->presets_[new_preset].gate_position);

  this->preset = new_preset;

  return true;
}

// TODO remove this method and use this->enable_preset_(this->saved_preset_);
void TionClimate::cancel_preset_(climate::ClimatePreset old_preset) {
  if (old_preset == climate::CLIMATE_PRESET_BOOST) {
    this->enable_preset_(this->saved_preset_);
  }
}
#endif

}  // namespace tion
}  // namespace esphome
#endif
