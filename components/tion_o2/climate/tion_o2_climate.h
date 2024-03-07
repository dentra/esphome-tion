#pragma once

#include "../tion_o2.h"
#include "../../tion/tion_climate_component.h"

namespace esphome {
namespace tion {

class TionO2Climate : public TionClimateComponent<TionO2Api> {
  using TionState = dentra::tion::TionState;
  using TionGatePosition = dentra::tion::TionGatePosition;

 public:
  explicit TionO2Climate(TionO2Api *api, TionVPortType vport_type) : TionClimateComponent(api, vport_type) {}

  void dump_config() override;

  void update_state(const TionState &state) override;

  TionGatePosition get_gate_position() const override { return TionGatePosition::NONE; }

  void reset_filter() { this->api_->reset_filter(this->state_); }

  void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, float target_temperature,
                             TionGatePosition gate_position) override;

  void control_buzzer_state(bool state) {}
};

}  // namespace tion
}  // namespace esphome
