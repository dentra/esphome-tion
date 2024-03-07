#pragma once

#include "../tion_lt.h"
#include "../../tion/tion_climate_component.h"

namespace esphome {
namespace tion {

class TionLtClimate : public TionLtClimateComponent<TionLtApi> {
  using TionState = dentra::tion::TionState;
  using TionGatePosition = dentra::tion::TionGatePosition;

 public:
  explicit TionLtClimate(TionLtApi *api, TionVPortType vport_type) : TionLtClimateComponent(api, vport_type) {}

  void dump_config() override;

  void update_state(const TionState &state) override;

  void set_gate_state(binary_sensor::BinarySensor *gate_state) { this->gate_state_ = gate_state; }

  void reset_filter() const { this->api_->reset_filter(this->state_); }

  void control_buzzer_state(bool state) {
    auto *call = this->make_api_call();
    call->set_sound_state(state);
    call->perform();
  }

  void control_led_state(bool state) {
    auto *call = this->make_api_call();
    call->set_led_state(state);
    call->perform();
  }

  void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, float target_temperature,
                             TionGatePosition gate_position) override;

  TionGatePosition get_gate_position() const override { return TionGatePosition::NONE; }

  void reset_errors() const { this->api_->reset_errors(this->state_); }

 protected:
  binary_sensor::BinarySensor *gate_state_{};
};

}  // namespace tion

}  // namespace esphome
