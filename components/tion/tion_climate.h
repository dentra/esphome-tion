#pragma once

#include "esphome/core/defines.h"

#ifdef USE_CLIMATE
#include "esphome/components/climate/climate.h"

namespace esphome {
namespace tion {

#ifdef TION_ENABLE_PRESETS
// default boost time - 10 minutes
#define DEFAULT_BOOST_TIME_SEC (10 * 60)

enum TionClimateGatePosition : uint8_t {
  TION_CLIMATE_GATE_POSITION_AUTO = 0,
  TION_CLIMATE_GATE_POSITION_OUTDOOR = 1,
  TION_CLIMATE_GATE_POSITION_INDOOR = 2,
  TION_CLIMATE_GATE_POSITION_MIXED = 3,
  TION_CLIMATE_GATE_POSITION__LAST = 4,  // NOLINT (bugprone-reserved-identifier)
};

struct TionPreset {
  uint8_t fan_speed;
  int8_t target_temperature;
  climate::ClimateMode mode;
  TionClimateGatePosition gate_position;
  bool is_initialized() const { return this->fan_speed != 0; }
} PACKED;

#define TION_MAX_PRESETS (climate::CLIMATE_PRESET_ACTIVITY + 1)
#endif  // TION_ENABLE_PRESETS

#define TION_DEFAULT_MAX_TEMPERATURE 25
#ifndef TION_MAX_TEMPERATURE
#define TION_MAX_TEMPERATURE TION_DEFAULT_MAX_TEMPERATURE
#endif
#if TION_MAX_TEMPERATURE > 30 || TION_MAX_TEMPERATURE < 20
#define TION_MAX_TEMPERATURE TION_DEFAULT_MAX_TEMPERATURE
#endif

#define TION_MAX_FAN_SPEED 6

class TionClimate : public climate::Climate {
 public:
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  virtual void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature,
                                     TionClimateGatePosition gate_position) = 0;
  virtual TionClimateGatePosition get_gate_position() const = 0;
#ifdef TION_ENABLE_PRESETS
  /**
   * Update default preset.
   * @param preset preset to update.
   * @param mode mode to update. set to climate::CLIMATE_MODE_AUTO for skip update.
   * @param fan_speed fan speed to update. set to 0 for skip update.
   * @param target_temperature target temperature to update. set to 0 for skip update.
   */
  bool update_preset(climate::ClimatePreset preset, climate::ClimateMode mode, uint8_t fan_speed = 0,
                     int8_t target_temperature = 0,
                     TionClimateGatePosition gate_position = TION_CLIMATE_GATE_POSITION_AUTO) {
    if (preset <= climate::CLIMATE_PRESET_NONE || preset > climate::CLIMATE_PRESET_ACTIVITY) {
      return false;
    }

    if (mode == climate::CLIMATE_MODE_OFF || mode == climate::CLIMATE_MODE_HEAT ||
        mode == climate::CLIMATE_MODE_FAN_ONLY) {
      this->presets_[preset].mode = mode;
    }

    if (fan_speed > 0 && fan_speed <= TION_MAX_FAN_SPEED) {
      this->presets_[preset].fan_speed = fan_speed;
    }

    if (target_temperature > 0 && target_temperature <= TION_MAX_TEMPERATURE) {
      this->presets_[preset].target_temperature = target_temperature;
    }

    if (gate_position >= TION_CLIMATE_GATE_POSITION_AUTO && gate_position < TION_CLIMATE_GATE_POSITION__LAST) {
      this->presets_[preset].gate_position = gate_position;
    }

    return true;
  }
#endif  // TION_ENABLE_PRESETS
  void dump_presets(const char *tag) const;

  uint8_t get_fan_speed() const { return this->fan_mode_to_speed_(this->custom_fan_mode); }

 protected:
  void set_fan_speed_(uint8_t fan_speed);

  uint8_t fan_mode_to_speed_(const optional<std::string> &fan_mode) const {
    if (fan_mode.has_value()) {
      return *fan_mode.value().c_str() - '0';
    }
    return 0;
  }

  std::string fan_speed_to_mode_(uint8_t fan_speed) const {
    char fan_mode[2] = {static_cast<char>(fan_speed + '0'), 0};
    return std::string(fan_mode);
  }
#ifdef TION_ENABLE_PRESETS
  bool enable_preset_(climate::ClimatePreset new_preset);
  void cancel_preset_(climate::ClimatePreset old_preset);
  void update_default_preset_() {
    this->presets_[climate::CLIMATE_PRESET_NONE].mode = this->mode;
    this->presets_[climate::CLIMATE_PRESET_NONE].fan_speed = this->get_fan_speed();
    this->presets_[climate::CLIMATE_PRESET_NONE].target_temperature = this->target_temperature;
    this->presets_[climate::CLIMATE_PRESET_NONE].gate_position = this->get_gate_position();
  }
  virtual bool enable_boost() = 0;
  virtual void cancel_boost() = 0;
  climate::ClimatePreset saved_preset_{climate::CLIMATE_PRESET_NONE};

  TionPreset presets_[TION_MAX_PRESETS] = {
      {},  // NONE, saved data
      {.fan_speed = 2,
       .target_temperature = 20,
       .mode = climate::CLIMATE_MODE_HEAT,
       .gate_position = TION_CLIMATE_GATE_POSITION_AUTO},  // HOME
      {.fan_speed = 1,
       .target_temperature = 10,
       .mode = climate::CLIMATE_MODE_FAN_ONLY,
       .gate_position = TION_CLIMATE_GATE_POSITION_AUTO},  // AWAY
      {.fan_speed = 6,
       .target_temperature = 10,
       .mode = climate::CLIMATE_MODE_FAN_ONLY,
       .gate_position = TION_CLIMATE_GATE_POSITION_AUTO},  // BOOST
      {.fan_speed = 2,
       .target_temperature = 23,
       .mode = climate::CLIMATE_MODE_HEAT,
       .gate_position = TION_CLIMATE_GATE_POSITION_AUTO},  // COMFORT
      {.fan_speed = 1,
       .target_temperature = 16,
       .mode = climate::CLIMATE_MODE_HEAT,
       .gate_position = TION_CLIMATE_GATE_POSITION_AUTO},  // ECO
      {.fan_speed = 1,
       .target_temperature = 18,
       .mode = climate::CLIMATE_MODE_HEAT,
       .gate_position = TION_CLIMATE_GATE_POSITION_AUTO},  // SLEEP
      {.fan_speed = 3,
       .target_temperature = 18,
       .mode = climate::CLIMATE_MODE_HEAT,
       .gate_position = TION_CLIMATE_GATE_POSITION_AUTO},  // ACTIVITY
  };

  void for_each_preset_(const std::function<void(climate::ClimatePreset index)> &fn) const {
    for (size_t i = climate::CLIMATE_PRESET_NONE + 1; i < TION_MAX_PRESETS; i++) {
      // "off" mean preset is disabled
      if (this->presets_[i].mode != climate::CLIMATE_MODE_OFF) {
        fn(static_cast<climate::ClimatePreset>(i));
      }
    }
  }

  void dump_preset_(const char *tag, climate::ClimatePreset index) const;
#endif  // TION_ENABLE_PRESETS
};

}  // namespace tion
}  // namespace esphome
#endif  // USE_CLIMATE
