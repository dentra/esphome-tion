#pragma once

#include "../tion_3s.h"
#include "../../tion/tion_climate_component.h"

namespace esphome {
namespace tion {

class Tion3sClimate : public TionClimateComponent<Tion3sApi> {
 public:
  explicit Tion3sClimate(Tion3sApi *api, TionVPortType vport_type) : TionClimateComponent(api, vport_type) {}

  void dump_config() override;

  void set_air_intake(select::Select *air_intake) { this->air_intake_ = air_intake; }

  void on_ready() {
    TionClimateComponent::on_ready();
    if (this->vport_type_ == TionVPortType::VPORT_UART && this->state_.firmware_version < 0x003C) {
      this->api_->request_command4();
    }
  }

  void update_state(const tion3s_state_t &state) override;
  void dump_state(const tion3s_state_t &state) const;

  void reset_filter() { this->api_->reset_filter(this->state_); }

  void control_buzzer_state(bool state) {
    ControlState control{};
    control.buzzer = state;
    this->control_state_(control);
  }

  void control_gate_position(tion3s_state_t::GatePosition gate_position);

  void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, float target_temperature,
                             TionClimateGatePosition gate_position) override;

  TionClimateGatePosition get_gate_position() const override {
    switch (this->get_gate_position_()) {
      case tion3s_state_t::GatePosition::GATE_POSITION_OUTDOOR:
        return TION_CLIMATE_GATE_POSITION_OUTDOOR;
      case tion3s_state_t::GatePosition::GATE_POSITION_INDOOR:
        return TION_CLIMATE_GATE_POSITION_INDOOR;
      case tion3s_state_t::GatePosition::GATE_POSITION_MIXED:
        return TION_CLIMATE_GATE_POSITION_MIXED;
      default:
        return TION_CLIMATE_GATE_POSITION_OUTDOOR;
    }
  }

  void enum_errors(uint32_t errors, const std::function<void(const std::string &)> &fn) const;

 protected:
  select::Select *air_intake_{};

  tion3s_state_t::GatePosition get_gate_position_() const {
    if (!this->batch_active_ && this->air_intake_) {
      auto active_index = this->air_intake_->active_index();
      if (active_index.has_value()) {
        return static_cast<tion3s_state_t::GatePosition>(*active_index);
      }
    }
    return this->state_.gate_position;
  }

  struct ControlState {
    optional<bool> power_state;
    optional<bool> heater_state;
    optional<uint8_t> fan_speed;
    optional<int8_t> target_temperature;
    optional<bool> buzzer;
    optional<tion3s_state_t::GatePosition> gate_position;
  };

  void control_state_(const ControlState &state);
};

}  // namespace tion
}  // namespace esphome
