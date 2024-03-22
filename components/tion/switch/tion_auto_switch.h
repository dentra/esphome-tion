#pragma once

#include "esphome/components/sensor/sensor.h"

#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif

#include "tion_switch.h"

namespace esphome {
namespace tion {

class TionAutoSwitch : public TionSwitch<property_controller::switch_::Auto> {
 public:
  explicit TionAutoSwitch(TionApiComponent *api) : TionSwitch(api) {}

  void register_co2_sensor(sensor::Sensor *co2) {
    co2->add_on_state_callback([this](float value) { this->update_co2_(value); });
  }

  void set_data(float kp, float ti, int db) { this->api()->set_auto_data(kp, ti, db); }
  void set_setpoint(uint16_t setpoint) { this->api()->set_auto_setpoint(setpoint); }
  void set_min_fan_speed(uint8_t min_fan_speed) { this->api()->set_auto_min_fan_speed(min_fan_speed); }
  void set_max_fan_speed(uint8_t max_fan_speed) { this->api()->set_auto_max_fan_speed(max_fan_speed); };
  void set_update_func(std::function<uint8_t(uint16_t current)> &&func) {
    this->api()->set_auto_update_func(std::move(func));
  }

 protected:
  void update_co2_(float value);

  dentra::tion::TionApiBase *api() const { return this->parent_->api(); }
};

}  // namespace tion
}  // namespace esphome
