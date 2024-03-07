#pragma once

#include "../tion_3s.h"
#include "../../tion/tion_climate_component.h"

namespace esphome {
namespace tion {

class Tion3sClimate : public TionClimateComponent<Tion3sApi> {
  using TionState = dentra::tion::TionState;
  using TionGatePosition = dentra::tion::TionGatePosition;

 public:
  explicit Tion3sClimate(Tion3sApi *api, TionVPortType vport_type) : TionClimateComponent(api, vport_type) {}

  void dump_config() override;

  void set_air_intake(select::Select *air_intake) { this->air_intake_ = air_intake; }

  // void on_ready() {
  //   TionClimateComponent::on_ready();
  //   if (this->vport_type_ == TionVPortType::VPORT_UART && this->state_.firmware_version < 0x003C) {
  //     this->api_->request_command4();
  //   }
  // }

  void update_state(const tion::TionState &state) override;

  void reset_filter() { this->api_->reset_filter(this->state_); }

  void control_buzzer_state(bool state) {
    auto *call = this->make_api_call();
    call->set_sound_state(state);
    call->perform();
  }

  void control_gate_position(TionGatePosition gate_position) {
    auto *call = this->make_api_call();
    call->set_gate_position(gate_position);
    call->perform();
  }

  void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, float target_temperature,
                             TionGatePosition gate_position) override;

  TionGatePosition get_gate_position() const override {
    if (!this->batch_active_ && this->air_intake_) {
      auto active_index = this->air_intake_->active_index();
      if (active_index.has_value()) {
        return static_cast<tion::TionGatePosition>(*active_index);
      }
    }
    return this->state_.gate_position;
  }

 protected:
  select::Select *air_intake_{};
};

}  // namespace tion
}  // namespace esphome
