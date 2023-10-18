#pragma once

#include "../tion_lt.h"
#include "../../tion/tion_climate_component.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

class TionLtClimate : public TionLtClimateComponent<TionLtApi> {
 public:
  explicit TionLtClimate(TionLtApi *api, TionVPortType vport_type) : TionLtClimateComponent(api, vport_type) {}

  void dump_config() override;

  void update_state(const tionlt_state_t &state) override;
  void dump_state(const tionlt_state_t &state) const;

  void reset_filter() const { this->api_->reset_filter(this->state_); }

  void control_buzzer_state(bool state) {
    this->control_climate_state(this->mode, this->get_fan_speed(), this->target_temperature, state, this->get_led_());
  }

  void control_led_state(bool state) {
    this->control_climate_state(this->mode, this->get_fan_speed(), this->target_temperature, this->get_buzzer_(),
                                state);
  }

  void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature) override {
    this->control_climate_state(mode, fan_speed, target_temperature, this->get_buzzer_(), this->get_led_());
  }

  void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature, bool buzzer,
                             bool led);

  void control_state(bool power_state, bool heater_state, uint8_t fan_speed, int8_t target_temperature, bool buzzer,
                     bool led);

  optional<int8_t> get_pcb_temperature() const {
    if (this->state_.is_initialized()) {
      return this->state_.pcb_temperature;
    }
    return {};
  }

  optional<int8_t> get_fan_time() const {
    if (this->state_.is_initialized()) {
      return this->state_.counters.fan_time;
    }
    return {};
  }

 protected:
  bool get_buzzer_() const { return this->buzzer_ ? this->buzzer_->state : this->state_.flags.sound_state; }
  bool get_led_() const { return this->led_ ? this->led_->state : this->state_.flags.led_state; }
};

}  // namespace tion

}  // namespace esphome
