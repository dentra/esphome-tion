#pragma once

#include "../tion-api/tion-api-lt.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

class TionLt : public TionClimateComponent<TionApiLt> {
 public:
  explicit TionLt(TionApiLt *api) : TionClimateComponent(api) {}

  void dump_config() override;

  void update_state(const tionlt_state_t &state) override;
  void dump_state(const tionlt_state_t &state) const;

  void reset_filter() const { this->api_->reset_filter(this->state_); }

  void control_buzzer_state(bool state) const {
    this->control_state(this->mode, this->get_fan_speed_(), this->target_temperature, state, this->get_led_());
  }

  void control_led_state(bool state) const {
    this->control_state(this->mode, this->get_fan_speed_(), this->target_temperature, this->get_buzzer_(), state);
  }

  void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature) const override {
    this->control_state(mode, fan_speed, target_temperature, this->get_buzzer_(), this->get_led_());
  }

  void control_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature, bool buzzer,
                     bool led) const;

 protected:
  bool get_buzzer_() const { return this->buzzer_ ? this->buzzer_->state : this->state_.flags.sound_state; }
  bool get_led_() const { return this->led_ ? this->led_->state : this->state_.flags.led_state; }
};

}  // namespace tion

}  // namespace esphome
