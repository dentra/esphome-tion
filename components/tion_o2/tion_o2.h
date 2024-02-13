#pragma once

#include "../tion-api/tion-api-o2.h"
#include "../tion/tion_controls.h"

namespace esphome {
namespace tion {

using TionO2Api = dentra::tion_o2::TionO2Api;

template<class T> class TionCall {
 public:
  explicit TionCall(T *parent) : parent_(parent) {}
  void set_fan_speed(uint8_t fan_speed) { this->fan_speed_ = fan_speed; }
  void set_target_temperature(int8_t target_temperature) { this->target_temperature_ = target_temperature; }
  void set_power_state(bool power_state) { this->power_state_ = power_state; }
  void set_heater_state(bool heater_state) { this->heater_state_ = heater_state; }

  const optional<uint8_t> &fan_speed() const { return this->fan_speed_; }
  const optional<bool> &power_state() const { return this->power_state_; }
  const optional<bool> &heater_state() const { return this->heater_state_; }
  const optional<uint8_t> &target_temperature() const { return this->target_temperature_; }

 protected:
  T *const parent_;
  optional<uint8_t> fan_speed_;
  optional<bool> power_state_;
  optional<bool> heater_state_;
  optional<int8_t> target_temperature_;
};

}  // namespace tion
}  // namespace esphome
