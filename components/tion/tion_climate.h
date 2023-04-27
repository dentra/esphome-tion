#pragma once

#include "esphome/components/climate/climate.h"

namespace esphome {
namespace tion {

#ifdef TION_ENABLE_PRESETS
// default boost time - 10 minutes
#define DEFAULT_BOOST_TIME_SEC 10 * 60

struct tion_preset_t {
  uint8_t fan_speed;
  int8_t target_temperature;
  climate::ClimateMode mode;
};
#endif

class TionClimate : public climate::Climate {
 public:
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  virtual void write_climate_state() = 0;
#ifdef TION_ENABLE_PRESETS
  /**
   * Update default preset.
   * @param preset preset to update.
   * @param mode mode to update. set to climate::CLIMATE_MODE_AUTO for skip update.
   * @param fan_speed fan speed to update. set to 0 for skip update.
   * @param target_temperature target temperature to update. set to 0 for skip update.
   */
  void update_preset(climate::ClimatePreset preset, climate::ClimateMode mode, uint8_t fan_speed,
                     int8_t target_temperature) {
    if (preset > climate::CLIMATE_PRESET_NONE && preset <= climate::CLIMATE_PRESET_ACTIVITY) {
      if (mode != climate::CLIMATE_MODE_AUTO) {
        this->presets_[preset].mode = mode;
      }
      if (fan_speed != 0) {
        this->presets_[preset].fan_speed = fan_speed;
      }
      if (target_temperature != 0) {
        this->presets_[preset].target_temperature = target_temperature;
      }
      this->supported_presets_.insert(preset);
    }
  }
#endif
 protected:
  uint8_t max_fan_speed_ = 6;
  void set_fan_speed_(uint8_t fan_speed);
  uint8_t get_fan_speed_() const { return this->fan_mode_to_speed_(this->custom_fan_mode); }

  static uint8_t fan_mode_to_speed_(const optional<std::string> &fan_mode) {
    if (fan_mode.has_value()) {
      return *fan_mode.value().c_str() - '0';
    }
    return 0;
  }

  static std::string fan_speed_to_mode_(uint8_t fan_speed) {
    char fan_mode[2] = {static_cast<char>(fan_speed + '0'), 0};
    return std::string(fan_mode);
  }
#ifdef TION_ENABLE_PRESETS
  bool enable_preset_(climate::ClimatePreset preset);
  void cancel_preset_(climate::ClimatePreset preset);
  virtual bool enable_boost_() = 0;
  virtual void cancel_boost_() = 0;
  climate::ClimatePreset saved_preset_{climate::CLIMATE_PRESET_NONE};

  tion_preset_t presets_[climate::CLIMATE_PRESET_ACTIVITY + 1] = {
      {},                                                                                  // NONE, saved data
      {.fan_speed = 2, .target_temperature = 20, .mode = climate::CLIMATE_MODE_HEAT},      // HOME
      {.fan_speed = 1, .target_temperature = 10, .mode = climate::CLIMATE_MODE_FAN_ONLY},  // AWAY
      {.fan_speed = 6, .target_temperature = 10, .mode = climate::CLIMATE_MODE_FAN_ONLY},  // BOOST
      {.fan_speed = 2, .target_temperature = 23, .mode = climate::CLIMATE_MODE_HEAT},      // COMFORT
      {.fan_speed = 1, .target_temperature = 16, .mode = climate::CLIMATE_MODE_HEAT},      // ECO
      {.fan_speed = 1, .target_temperature = 18, .mode = climate::CLIMATE_MODE_HEAT},      // SLEEP
      {.fan_speed = 3, .target_temperature = 18, .mode = climate::CLIMATE_MODE_HEAT},      // ACTIVITY
  };

  std::set<climate::ClimatePreset> supported_presets_{};
#endif
};

}  // namespace tion
}  // namespace esphome
