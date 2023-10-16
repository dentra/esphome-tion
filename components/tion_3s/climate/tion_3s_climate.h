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

  int8_t get_unknown_temperature() const { return this->state_.unknown_temperature; }

  void control_buzzer_state(bool state) {
    this->control_climate_state(this->mode, this->get_fan_speed_(), this->target_temperature, state,
                                this->get_gate_position_());
  }

  void control_gate_position(tion3s_state_t::GatePosition gate_position);

  void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature) override;

  void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature, bool buzzer,
                             tion3s_state_t::GatePosition gate_position);

  void control_state(bool power_state, bool heater_state, uint8_t fan_speed, int8_t target_temperature, bool buzzer,
                     tion3s_state_t::GatePosition gate_position);

 protected:
  select::Select *air_intake_{};

  bool get_buzzer_() const { return this->buzzer_ ? this->buzzer_->state : this->state_.flags.sound_state; }

  tion3s_state_t::GatePosition get_gate_position_() const {
    if (this->air_intake_) {
      auto active_index = this->air_intake_->active_index();
      if (active_index.has_value()) {
        return static_cast<tion3s_state_t::GatePosition>(*active_index);
      }
    }
    return this->state_.gate_position;
  }
};

}  // namespace tion
}  // namespace esphome
