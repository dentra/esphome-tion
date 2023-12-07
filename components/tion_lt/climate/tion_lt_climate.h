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

  void set_gate_state(binary_sensor::BinarySensor *gate_state) { this->gate_state_ = gate_state; }

  void reset_filter() const { this->api_->reset_filter(this->state_); }

  void control_buzzer_state(bool state) {
    ControlState control{};
    control.buzzer = state;
    this->control_state_(control);
  }

  void control_led_state(bool state) {
    ControlState control{};
    control.led = state;
    this->control_state_(control);
  }

  void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature,
                             TionClimateGatePosition gate_position) override;

  TionClimateGatePosition get_gate_position() const override { return TION_CLIMATE_GATE_POSITION_NONE; }

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
  binary_sensor::BinarySensor *gate_state_{};

  struct ControlState {
    optional<bool> power_state;
    optional<bool> heater_state;
    optional<uint8_t> fan_speed;
    optional<int8_t> target_temperature;
    optional<bool> buzzer;
    optional<bool> led;
  };

  void control_state_(const ControlState &state);
};

}  // namespace tion

}  // namespace esphome
