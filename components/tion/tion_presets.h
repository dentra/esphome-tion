#pragma once

#include "esphome/core/defines.h"

#include "esphome/components/sensor/sensor.h"
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif

#include "tion_defines.h"
#include "tion_helpers.h"

namespace esphome {
namespace tion {

#ifndef TION_ENABLE_PRESETS
class TionPresets {
 public:
  void dump_presets(const char *tag) const {}
  void setup_presets() const {}
  void add_presets(climate::ClimateTraits&traits)const{}
};
#endif

}  // namespace tion
}  // namespace esphome

#ifdef TION_ENABLE_PRESETS

// default boost time - 10 minutes
#define DEFAULT_BOOST_TIME_SEC (10 * 60)

#define TION_MAX_PRESETS (climate::CLIMATE_PRESET_ACTIVITY + 1)

namespace esphome {
namespace tion {

struct TionClimatePresetData {
  uint8_t fan_speed;
  int8_t target_temperature;
  climate::ClimateMode mode;
  TionClimateGatePosition gate_position;
  bool is_initialized() const { return this->fan_speed != 0; }
  bool is_enabled() const { return this->mode != climate::CLIMATE_MODE_OFF; }
} PACKED;

class TionPresets {
 public:
  void set_boost_time(number::Number *boost_time) { this->boost_time_ = boost_time; }
  void set_boost_time_left(sensor::Sensor *boost_time_left) { this->boost_time_left_ = boost_time_left; }

  void setup_presets();
  void add_presets(climate::ClimateTraits&traits);

  /**
   * Update default preset.
   * @param preset preset to update.
   * @param mode mode to update. set to climate::CLIMATE_MODE_AUTO for skip update.
   * @param fan_speed fan speed to update. set to 0 for skip update.
   * @param target_temperature target temperature to update. set to 0 for skip update.
   */
  bool update_preset(climate::ClimatePreset preset, climate::ClimateMode mode, uint8_t fan_speed = 0,
                     int8_t target_temperature = 0,
                     TionClimateGatePosition gate_position = TION_CLIMATE_GATE_POSITION_NONE) {
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

    if (target_temperature >= TION_MIN_TEMPERATURE && target_temperature <= TION_MAX_TEMPERATURE) {
      this->presets_[preset].target_temperature = target_temperature;
    }

    if (gate_position < TION_CLIMATE_GATE_POSITION__LAST) {
      this->presets_[preset].gate_position = gate_position;
    }

    return true;
  }
  void dump_presets(const char *tag) const;

  virtual TionClimateGatePosition get_gate_position() const = 0;

 protected:
  number::Number *boost_time_{};
  sensor::Sensor *boost_time_left_{};
  ESPPreferenceObject boost_rtc_;

  bool presets_enable_boost_(Component *component, climate::Climate *climate);
  void presets_cancel_boost_(Component *component, climate::Climate *climate);

  TionClimatePresetData *presets_enable_preset_(climate::ClimatePreset new_preset, Component *component,
                                                climate::Climate *climate);
  TionClimatePresetData *presets_cancel_preset_(climate::ClimatePreset old_preset, Component *component,
                                                climate::Climate *climate);

#ifdef USE_API
  void update_preset_service_(std::string preset, std::string mode, int32_t fan_speed, int32_t target_temperature,
                              std::string gate_position);
  ESPPreferenceObject presets_rtc_;
#endif

  climate::ClimatePreset saved_preset_{climate::CLIMATE_PRESET_NONE};

  TionClimatePresetData presets_[TION_MAX_PRESETS] = {
      {},  // NONE, saved data
      {.fan_speed = 2,
       .target_temperature = 20,
       .mode = climate::CLIMATE_MODE_HEAT,
       .gate_position = TION_CLIMATE_GATE_POSITION_NONE},  // HOME
      {.fan_speed = 1,
       .target_temperature = 10,
       .mode = climate::CLIMATE_MODE_FAN_ONLY,
       .gate_position = TION_CLIMATE_GATE_POSITION_NONE},  // AWAY
      {.fan_speed = TION_MAX_FAN_SPEED,
       .target_temperature = 10,
       .mode = climate::CLIMATE_MODE_FAN_ONLY,
       .gate_position = TION_CLIMATE_GATE_POSITION_NONE},  // BOOST
      {.fan_speed = 2,
       .target_temperature = 23,
       .mode = climate::CLIMATE_MODE_HEAT,
       .gate_position = TION_CLIMATE_GATE_POSITION_NONE},  // COMFORT
      {.fan_speed = 1,
       .target_temperature = 16,
       .mode = climate::CLIMATE_MODE_HEAT,
       .gate_position = TION_CLIMATE_GATE_POSITION_NONE},  // ECO
      {.fan_speed = 1,
       .target_temperature = 18,
       .mode = climate::CLIMATE_MODE_HEAT,
       .gate_position = TION_CLIMATE_GATE_POSITION_NONE},  // SLEEP
      {.fan_speed = 3,
       .target_temperature = 18,
       .mode = climate::CLIMATE_MODE_HEAT,
       .gate_position = TION_CLIMATE_GATE_POSITION_NONE},  // ACTIVITY
  };

  void for_each_preset_(const std::function<void(climate::ClimatePreset index)> &fn) const {
    for (size_t i = climate::CLIMATE_PRESET_NONE + 1; i < TION_MAX_PRESETS; i++) {
      if (this->presets_[i].is_enabled()) {
        fn(static_cast<climate::ClimatePreset>(i));
      }
    }
  }

  void dump_preset_(const char *tag, climate::ClimatePreset index) const;

  void update_default_preset_(climate::Climate *climate);

  /// returns boost time in seconds.
  uint32_t get_boost_time_() const {
    if (this->boost_time_ == nullptr) {
      return DEFAULT_BOOST_TIME_SEC;
    }
    if (this->boost_time_->traits.get_unit_of_measurement()[0] == 's') {
      return this->boost_time_->state;
    }
    return this->boost_time_->state * 60;
  }
};

}  // namespace tion
}  // namespace esphome
#endif  // TION_ENABLE_PRESETS
