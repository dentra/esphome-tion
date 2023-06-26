#pragma once

#include "esphome/components/select/select.h"

#include "../tion-api/tion-api-3s.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

using TionApi3s = dentra::tion::TionApi3s;

class Tion3s : public TionClimateComponent<TionApi3s> {
 public:
  explicit Tion3s(TionApi3s *api) : TionClimateComponent(api) {}

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

  void reset_filter() {
    this->api_->reset_filter(this->state_);
    if (this->vport_type_ == TionVPortType::VPORT_UART) {
      this->defer([this]() { this->api_->request_command4(); });
    }
  }

  int8_t get_unknown_temperature() const { return this->state_.unknown_temperature; }

  void control_buzzer_state(bool state) const {
    this->control_state(this->mode, this->get_fan_speed_(), this->target_temperature, state,
                        this->get_gate_position_());
  }

  void control_air_intake(uint8_t air_intake) const {
    this->control_state(this->mode, this->get_fan_speed_(), this->target_temperature, this->get_buzzer_(), air_intake);
  }

  void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature) const override {
    this->control_state(mode, fan_speed, target_temperature, this->get_buzzer_(), this->get_gate_position_());
  }

  void control_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature, bool buzzer,
                     uint8_t gate_position) const;

 protected:
  select::Select *air_intake_{};

  bool get_buzzer_() const { return this->buzzer_ ? this->buzzer_->state : this->state_.flags.sound_state; }

  uint8_t get_gate_position_() const {
    if (this->air_intake_) {
      auto active_index = this->air_intake_->active_index();
      if (active_index.has_value()) {
        return *active_index;
      }
    }
    return this->state_.gate_position;
  }
};

class Tion3sAirIntakeSelect : public select::Select {
 public:
  explicit Tion3sAirIntakeSelect(Tion3s *parent) : parent_(parent) {}
  void control(const std::string &value) override { this->parent_->control_air_intake(*this->index_of(value)); }

 protected:
  Tion3s *parent_;
};

}  // namespace tion
}  // namespace esphome
