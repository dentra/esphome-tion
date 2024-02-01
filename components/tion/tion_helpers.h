#pragma once

#include "esphome/core/helpers.h"

#ifdef USE_CLIMATE
#include "esphome/components/climate/climate.h"
#endif

namespace esphome {
namespace tion {

inline uint8_t fan_mode_to_speed(const char *fan_mode) { return *fan_mode - '0'; }
inline uint8_t fan_mode_to_speed(const std::string &fan_mode) { return fan_mode_to_speed(fan_mode.c_str()); }
inline uint8_t fan_mode_to_speed(const optional<std::string> &fan_mode) {
  return fan_mode.has_value() ? fan_mode_to_speed(fan_mode.value()) : 0;
}

inline std::string fan_speed_to_mode(uint8_t fan_speed) {
  char fan_mode[2] = {static_cast<char>(fan_speed + '0'), 0};
  return std::string(fan_mode);
}

enum class TionGatePosition : uint8_t { NONE = 0, OUTDOOR = 1, INDOOR = 2, MIXED = 3, _LAST = 4 };

#ifdef USE_CLIMATE
using TionClimateGatePosition = TionGatePosition;
#endif

template<typename mode_type, mode_type off_value> struct TionPresetData {
  uint8_t fan_speed;
  int8_t target_temperature;
  mode_type mode;
  TionGatePosition gate_position;
  bool is_initialized() const { return this->fan_speed != 0; }
  bool is_enabled() const { return this->mode != off_value; }
} PACKED;

}  // namespace tion
}  // namespace esphome
