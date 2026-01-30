#include "esphome/core/log.h"

#include "tion_climate.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_climate";

constexpr static const auto FAN_MODE_LABELS = {"1", "2", "3", "4", "5", "6"};

// ВАЖНО: fan_mode не должен быть nullptr
inline uint8_t fan_mode_to_speed(const char *fan_mode) { return *fan_mode - '0'; }
inline uint8_t fan_mode_to_speed(const StringRef &fan_mode) { return fan_mode_to_speed(fan_mode.c_str()); }
// ВАЖНО: минимальная скорость 1
inline const char *speed_to_fan_mode(uint8_t fan_speed) { return *(FAN_MODE_LABELS.begin() + (fan_speed - 1)); }

const char *preset_to_string(climate::ClimatePreset preset) {
#ifndef USE_STORE_LOG_STR_IN_FLASH
  return LOG_STR_ARG(climate::climate_preset_to_string(preset));
#else
  switch (preset) {
    case climate::CLIMATE_PRESET_NONE:
      return "NONE";
    case climate::CLIMATE_PRESET_HOME:
      return "HOME";
    case climate::CLIMATE_PRESET_ECO:
      return "ECO";
    case climate::CLIMATE_PRESET_AWAY:
      return "AWAY";
    case climate::CLIMATE_PRESET_BOOST:
      return "BOOST";
    case climate::CLIMATE_PRESET_COMFORT:
      return "COMFORT";
    case climate::CLIMATE_PRESET_SLEEP:
      return "SLEEP";
    case climate::CLIMATE_PRESET_ACTIVITY:
      return "ACTIVITY";
    default:
      return "UNKNOWN";
  }
#endif
}

inline bool std_preset_is_invalid(climate::ClimatePreset preset) { return static_cast<int8_t>(preset) < 0; }

climate::ClimatePreset std_preset_find(const char *preset) {
  for (climate::ClimatePreset i = climate::CLIMATE_PRESET_NONE; i <= climate::CLIMATE_PRESET_ACTIVITY;
       i = static_cast<climate::ClimatePreset>(i + 1)) {
    const auto preset_climate = preset_to_string(i);
    if (strcasecmp(preset, preset_climate) == 0) {
      return i;
    }
  }
  return static_cast<climate::ClimatePreset>(-1);
}

void TionClimate::setup() {
  ESP_LOGD(TAG, "Setting up %s...", this->get_name().c_str());

  this->parent_->add_on_state_callback([this](const TionState *state) {
    if (state) {
      this->set_has_state(true);
      this->on_state_(*state);
    } else {
      this->set_has_state(false);
    }
  });
}

climate::ClimateTraits TionClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.set_visual_min_temperature(this->parent_->traits().min_target_temperature);
  traits.set_visual_max_temperature(this->parent_->traits().max_target_temperature);
  traits.set_visual_temperature_step(1.0f);
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_FAN_ONLY,
  });
  if (this->options_.enable_heat_cool) {
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT_COOL);
  }
  if (this->options_.enable_fan_auto && this->parent_->api()->auto_is_valid()) {
    traits.add_supported_fan_mode(climate::CLIMATE_FAN_AUTO);
  }
  if (this->options_.enable_fan_off && this->parent_->traits().supports_kiv) {
    traits.add_supported_fan_mode(climate::CLIMATE_FAN_OFF);
  }

  // максимальная скорость может быть 4 или 6, поэтому собираем массив динамически
  std::vector<const char *> fan_modes;
  for (uint8_t i = 0, max_fan_speed =
                          std::min(this->parent_->traits().max_fan_speed, static_cast<uint8_t>(FAN_MODE_LABELS.size()));
       i < max_fan_speed; i++) {
    fan_modes.push_back(speed_to_fan_mode(i + 1));
  }
  traits.set_supported_custom_fan_modes(fan_modes);

  if (this->parent_->api()->has_presets()) {
    std::vector<const char *> presets;
    for (auto &&p : this->parent_->api()->get_presets()) {
      const auto preset = std_preset_find(p);
      if (std_preset_is_invalid(preset)) {
        presets.push_back(p);
      } else {
        traits.add_supported_preset(preset);
      }
    }
    if (!presets.empty()) {
      traits.set_supported_custom_presets(presets);
    }
  }

  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_ACTION);
  return traits;
}

void TionClimate::dump_config() {
  LOG_CLIMATE("", "Tion Climate", this);
  this->dump_traits_(TAG);
}

void TionClimate::control(const climate::ClimateCall &call) {
  auto *tion = this->parent_->make_call();

  if (this->parent_->api()->has_presets()) {
    // в ESPHome 2025.11 preset всегда будет содержать ClimatePreset пресет, даже если его устанавливали строкой

    if (call.get_preset().has_value()) {
      const auto preset = preset_to_string(*call.get_preset());
      TION_C_LOGD(TAG, "Set preset %s", preset);
      this->parent_->api()->enable_preset(preset, tion);
    } else if (call.has_custom_preset()) {
      const auto preset = call.get_custom_preset();
      TION_C_LOGD(TAG, "Set custom preset %s", preset);
      this->parent_->api()->enable_preset(preset.c_str(), tion);
    }
  }

  if (call.get_mode().has_value()) {
    const auto mode = *call.get_mode();
    TION_C_LOGD(TAG, "Set mode %s", LOG_STR_ARG(climate::climate_mode_to_string(mode)));
    if (mode == climate::CLIMATE_MODE_OFF) {
      tion->set_power_state(false);
    } else {
      tion->set_power_state(true);
      if (mode != climate::CLIMATE_MODE_HEAT_COOL) {
        tion->set_heater_state(mode == climate::CLIMATE_MODE_HEAT);
      }
    }
  }

  if (call.get_fan_mode().has_value()) {
    const auto fan_mode = *call.get_fan_mode();
    if (this->options_.enable_fan_auto && fan_mode == climate::CLIMATE_FAN_AUTO) {
      TION_C_LOGD(TAG, "Set fan speed AUTO");
      tion->set_auto_state(true);
    }
    if (this->parent_->traits().supports_kiv && fan_mode == climate::CLIMATE_FAN_OFF) {
      TION_C_LOGD(TAG, "Set fan speed OFF");
      tion->set_fan_speed(0);
    }
  }

  if (call.has_custom_fan_mode()) {
    const auto fan_mode = call.get_custom_fan_mode();
    const auto fan_speed = fan_mode_to_speed(fan_mode);
    TION_C_LOGD(TAG, "Set fan speed %u", fan_speed);
    tion->set_fan_speed(fan_speed);
  }

  if (call.get_target_temperature().has_value()) {
    const int8_t target_temperature = *call.get_target_temperature();
    TION_C_LOGD(TAG, "Set target temperature %d °C", target_temperature);
    tion->set_target_temperature(target_temperature);
  }

  tion->perform();
}

void TionClimate::on_state_(const TionState &state) {
  bool has_changes = false;

  climate::ClimateMode mode;
  climate::ClimateAction action;
  if (!state.power_state) {
    const auto is_auto =
        this->options_.enable_fan_auto && state.auto_state && this->parent_->api()->get_auto_min_fan_speed() == 0;
    mode = climate::CLIMATE_MODE_OFF;
    action = is_auto ? climate::CLIMATE_ACTION_IDLE : climate::CLIMATE_ACTION_OFF;
  } else if (state.heater_state) {
    mode = climate::CLIMATE_MODE_HEAT;
    action = state.is_heating(this->parent_->traits())  //-//
                 ? climate::CLIMATE_ACTION_HEATING
                 : climate::CLIMATE_ACTION_FAN;
  } else {
    mode = climate::CLIMATE_MODE_FAN_ONLY;
    action = state.fan_speed == 0 ? climate::CLIMATE_ACTION_IDLE : climate::CLIMATE_ACTION_FAN;
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

  if (this->set_fan_speed_(state)) {
    has_changes = true;
  }

  if (this->parent_->api()->has_presets()) {
    const auto active_preset = this->parent_->api()->get_active_preset_name();
    if (const auto climate_preset = std_preset_find(active_preset); !std_preset_is_invalid(climate_preset)) {
      has_changes |= this->set_preset_(climate_preset);
    } else if (!this->has_custom_preset() || strcasecmp(this->get_custom_preset().c_str(), active_preset) != 0) {
      has_changes |= this->set_custom_preset_(active_preset);
    }
  }

  if (this->parent_->get_force_update() || has_changes) {
    this->publish_state();
  }
}

bool TionClimate::set_fan_speed_(const TionState &state) {
  // спец обработка AUTO, только если включена поддержка режима AUTO
  if (this->options_.enable_fan_auto && state.auto_state) {
    // только если значение не выставлено или не AUTO
    return this->set_fan_mode_(climate::CLIMATE_FAN_AUTO);
  }

  // спец обработка скорости 0
  if (state.fan_speed == 0) {
    // только если поддерживается скорость 0
    if (this->parent_->traits().supports_kiv) {
      // TODO что будет если вернулись из авто с 0 скоростью, а enable_fan_off_=false
      // только если значение не выставлено или не OFF
      return this->set_fan_mode_(climate::CLIMATE_FAN_OFF);
    }
    if (this->mode != climate::CLIMATE_MODE_OFF) {
      TION_C_LOGW(TAG, "Unsupported zero fan speed");
    }
    return false;
  }

  if (state.fan_speed > this->parent_->traits().max_fan_speed) {
    TION_C_LOGW(TAG, "Unsupported fan speed %u of %u", state.fan_speed, this->parent_->traits().max_fan_speed);
    return false;
  }

  if ((!this->has_custom_fan_mode() || fan_mode_to_speed(this->get_custom_fan_mode()) != state.fan_speed) &&
      state.fan_speed <= FAN_MODE_LABELS.size()) {
    return this->set_custom_fan_mode_(speed_to_fan_mode(state.fan_speed));
  }

  return false;
}

}  // namespace tion
}  // namespace esphome
