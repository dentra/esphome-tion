#pragma once

#include "../../tion-api/tion-api-4s.h"
#include "../../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

using TionApi4s = dentra::tion::TionApi4s;

class Tion4s : public TionClimateComponent<TionApi4s> {
 public:
  explicit Tion4s(TionApi4s *api) : TionClimateComponent(api) {
#ifdef TION_ENABLE_SCHEDULER
    this->api_->on_time.set<Tion4s, &Tion4s::on_time>(*this);
    this->api_->on_timer.set<Tion4s, &Tion4s::on_timer>(*this);
    this->api_->on_timers_state.set<Tion4s, &Tion4s::on_timers_state>(*this);
#endif
  }

  void dump_config() override;

  void set_recirculation(switch_::Switch *recirculation) { this->recirculation_ = recirculation; }

#ifdef TION_ENABLE_PRESETS
  void update() override {
    TionClimateComponent::update();
    if (this->vport_type_ == TionVPortType::VPORT_BLE) {
      this->api_->request_turbo();
    }
  }
#endif

  // #ifdef TION_ENABLE_SCHEDULER
  //   void on_ready() {
  //     TionClimateComponent::on_ready();
  //     // scheduler specific init commands
  //     this->api_->request_time();
  //   }
  // #endif

#ifdef TION_ENABLE_PRESETS
  void on_turbo(const tion4s_turbo_t &turbo, const uint32_t request_id);
#endif
#ifdef TION_ENABLE_SCHEDULER
  void on_time(const time_t time, const uint32_t request_id);
  void on_timer(const uint8_t timer_id, const tion4s_timer_t &timer, uint32_t request_id);
  void on_timers_state(const tion4s_timers_state_t &timers_state, uint32_t request_id);
  void dump_timers();
  void reset_timers();
#endif
  void update_state(const tion4s_state_t &state) override;
  void dump_state(const tion4s_state_t &state) const;

  void reset_filter() const { this->api_->reset_filter(this->state_); }

  void control_buzzer_state(bool state) {
    this->control_state(this->mode, this->get_fan_speed_(), this->target_temperature, state, this->get_led_(),
                        this->get_recirculation_());
  }

  void control_led_state(bool state) {
    this->control_state(this->mode, this->get_fan_speed_(), this->target_temperature, this->get_buzzer_(), state,
                        this->get_recirculation_());
  }

  void control_recirculation_state(bool state) {
    this->control_state(this->mode, this->get_fan_speed_(), this->target_temperature, this->get_buzzer_(),
                        this->get_led_(), state);
  }

  void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature) override {
    this->control_state(mode, fan_speed, target_temperature, this->get_buzzer_(), this->get_led_(),
                        this->get_recirculation_());
  }

  void control_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature, bool buzzer, bool led,
                     bool recirculation);

  optional<int8_t> get_pcb_ctl_temperature() const {
    if (this->state_.is_initialized()) {
      return this->state_.pcb_ctl_temperature;
    }
    return {};
  }

  optional<int8_t> get_pcb_pwr_temperature() const {
    if (this->state_.is_initialized()) {
      return this->state_.pcb_pwr_temperature;
    }
    return {};
  }

 protected:
  uint32_t request_id_{};
  switch_::Switch *recirculation_{};
#ifdef TION_ENABLE_PRESETS
  bool enable_boost_() override;
  void cancel_boost_() override;
#endif
  bool get_buzzer_() const { return this->buzzer_ ? this->buzzer_->state : this->state_.flags.sound_state; }
  bool get_led_() const { return this->led_ ? this->led_->state : this->state_.flags.led_state; }
  bool get_recirculation_() const {
    return this->recirculation_ ? this->recirculation_->state
                                : this->state_.gate_position == tion4s_state_t::GATE_POSITION_RECIRCULATION;
  }
};

template<class parent_t> class TionRecirculationSwitch : public Parented<parent_t>, public switch_::Switch {
 public:
  explicit TionRecirculationSwitch(parent_t *parent) : Parented<parent_t>(parent) {}
  void write_state(bool state) override { this->parent_->control_recirculation_state(state); }
};

}  // namespace tion

}  // namespace esphome
