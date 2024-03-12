#pragma once

#include "esphome/core/helpers.h"

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

}  // namespace tion
}  // namespace esphome
