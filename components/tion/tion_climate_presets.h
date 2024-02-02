#pragma once
#include "esphome/core/defines.h"
#ifdef USE_CLIMATE

#include "esphome/components/sensor/sensor.h"
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif

#include "tion_defines.h"
#include "tion_helpers.h"

namespace esphome {
namespace tion {

#ifndef TION_ENABLE_PRESETS
class TionClimatePresets {
 public:
  void dump_presets(const char *tag) const {}
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

using TionClimatePresetData = TionPresetData<climate::ClimateMode, climate::CLIMATE_MODE_OFF>;

class TionClimatePresets {
 public:
  void set_boost_time(number::Number *boost_time) { this->presets_boost_time_ = boost_time; }
  void set_boost_time_left(sensor::Sensor *boost_time_left) { this->presets_boost_time_left_ = boost_time_left; }

  void setup_presets();
  void add_presets(climate::ClimateTraits &traits);
  void dump_presets(const char *tag) const;

  virtual TionGatePosition get_gate_position() const = 0;

  /**
   * Update default preset.
   * @param preset preset to update.
   * @param mode mode to update. set to climate::CLIMATE_MODE_AUTO for skip update.
   * @param fan_speed fan speed to update. set to 0 for skip update.
   * @param target_temperature target temperature to update. set to 0 for skip update.
   */
  bool update_preset(climate::ClimatePreset preset, climate::ClimateMode mode, uint8_t fan_speed = 0,
                     int8_t target_temperature = 0, TionGatePosition gate_position = TionGatePosition::NONE) {
    if (preset <= climate::CLIMATE_PRESET_NONE || preset > climate::CLIMATE_PRESET_ACTIVITY) {
      return false;
    }

    if (mode == climate::CLIMATE_MODE_OFF || mode == climate::CLIMATE_MODE_HEAT ||
        mode == climate::CLIMATE_MODE_FAN_ONLY) {
      this->presets_data_[preset].mode = mode;
    }

    if (fan_speed > 0 && fan_speed <= TION_MAX_FAN_SPEED) {
      this->presets_data_[preset].fan_speed = fan_speed;
    }

    if (target_temperature >= TION_MIN_TEMPERATURE && target_temperature <= TION_MAX_TEMPERATURE) {
      this->presets_data_[preset].target_temperature = target_temperature;
    }

    if (gate_position < TionGatePosition::_LAST) {
      this->presets_data_[preset].gate_position = gate_position;
    }

    return true;
  }

 protected:
  number::Number *presets_boost_time_{};
  sensor::Sensor *presets_boost_time_left_{};
  ESPPreferenceObject presets_boost_rtc_;

  bool presets_enable_boost_timer_(Component *component, climate::Climate *climate);
  void presets_cancel_boost_timer_(Component *component);
  // partially activate preset, caller must perform actions with returned result if it is not null.
  TionClimatePresetData *presets_activate_preset_(climate::ClimatePreset new_preset, Component *component,
                                                  climate::Climate *climate);
  // Cancel boost and activate saved preset with making climate call.
  void presets_cancel_boost_(climate::Climate *climate);

#ifdef USE_API
  void presets_update_service_(std::string preset, std::string mode, int32_t fan_speed, int32_t target_temperature,
                               std::string gate_position);
  ESPPreferenceObject presets_data_rtc_;
#endif

  climate::ClimatePreset presets_saved_preset_{climate::CLIMATE_PRESET_NONE};

  TionClimatePresetData presets_data_[TION_MAX_PRESETS] = {
      {},  // NONE, saved data
      {.fan_speed = 2,
       .target_temperature = 20,
       .mode = climate::CLIMATE_MODE_HEAT,
       .gate_position = TionGatePosition::NONE},  // HOME
      {.fan_speed = 1,
       .target_temperature = 10,
       .mode = climate::CLIMATE_MODE_FAN_ONLY,
       .gate_position = TionGatePosition::NONE},  // AWAY
      {.fan_speed = TION_MAX_FAN_SPEED,
       .target_temperature = 10,
       .mode = climate::CLIMATE_MODE_FAN_ONLY,
       .gate_position = TionGatePosition::NONE},  // BOOST
      {.fan_speed = 2,
       .target_temperature = 23,
       .mode = climate::CLIMATE_MODE_HEAT,
       .gate_position = TionGatePosition::NONE},  // COMFORT
      {.fan_speed = 1,
       .target_temperature = 16,
       .mode = climate::CLIMATE_MODE_HEAT,
       .gate_position = TionGatePosition::NONE},  // ECO
      {.fan_speed = 1,
       .target_temperature = 18,
       .mode = climate::CLIMATE_MODE_HEAT,
       .gate_position = TionGatePosition::NONE},  // SLEEP
      {.fan_speed = 3,
       .target_temperature = 18,
       .mode = climate::CLIMATE_MODE_HEAT,
       .gate_position = TionGatePosition::NONE},  // ACTIVITY
  };

  void presets_for_each_(const std::function<void(climate::ClimatePreset index)> &fn) const {
    for (size_t i = climate::CLIMATE_PRESET_NONE + 1; i < TION_MAX_PRESETS; i++) {
      if (this->presets_data_[i].is_enabled()) {
        fn(static_cast<climate::ClimatePreset>(i));
      }
    }
  }

  bool has_presets() const {
    auto has_presets = false;
    this->presets_for_each_([&has_presets](auto index) { has_presets = true; });
    return has_presets;
  }

  void presets_dump_preset_(const char *tag, climate::ClimatePreset index) const;

  void presets_save_default_(climate::Climate *climate);

  /// returns boost time in seconds.
  uint32_t get_boost_time_() const {
    if (this->presets_boost_time_ == nullptr) {
      return DEFAULT_BOOST_TIME_SEC;
    }
    if (this->presets_boost_time_->traits.get_unit_of_measurement()[0] == 's') {
      return this->presets_boost_time_->state;
    }
    return this->presets_boost_time_->state * 60;
  }
};

}  // namespace tion
}  // namespace esphome
#endif  // TION_ENABLE_PRESETS
#endif  // USE_CLIMATE
