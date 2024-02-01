#pragma once

#include "esphome/components/switch/switch.h"

#include "../tion_4s.h"
#include "../../tion/tion_climate_component.h"

namespace esphome {
namespace tion {

class Tion4sClimate : public TionLtClimateComponent<TionApi4s> {
 public:
  explicit Tion4sClimate(TionApi4s *api, TionVPortType vport_type) : TionLtClimateComponent(api, vport_type) {
#ifdef TION_ENABLE_SCHEDULER
    this->api_->on_time.set<Tion4sClimate, &Tion4sClimate::on_time>(*this);
    this->api_->on_timer.set<Tion4sClimate, &Tion4sClimate::on_timer>(*this);
    this->api_->on_timers_state.set<Tion4sClimate, &Tion4sClimate::on_timers_state>(*this);
#endif
  }

  void dump_config() override;

  void set_recirculation(switch_::Switch *recirculation) { this->recirculation_ = recirculation; }

#ifdef TION_ENABLE_PRESETS
  void update() override {
    TionLtClimateComponent::update();
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
  void on_turbo(const tion4s_turbo_t &turbo, uint32_t request_id);
#endif
#ifdef TION_ENABLE_SCHEDULER
  void on_time(time_t time, uint32_t request_id);
  void on_timer(uint8_t timer_id, const tion4s_timer_t &timer, uint32_t request_id);
  void on_timers_state(const tion4s_timers_state_t &timers_state, uint32_t request_id);
  void dump_timers();
  void reset_timers();
#endif
  void update_state(const tion4s_state_t &state) override;
  void dump_state(const tion4s_state_t &state) const;

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

  void control_recirculation_state(bool state);
  void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, float target_temperature,
                             TionGatePosition gate_position) override;

  TionGatePosition get_gate_position() const override {
    switch (this->get_gate_position_()) {
      case tion4s_state_t::GATE_POSITION_INFLOW:
        return TionGatePosition::OUTDOOR;
      case tion4s_state_t::GATE_POSITION_RECIRCULATION:
        return TionGatePosition::INDOOR;
      default:
        return TionGatePosition::OUTDOOR;
    }
  }

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

  void enum_errors(uint32_t errors, const std::function<void(const std::string &)> &fn) const;

  void reset_errors() const { this->api_->reset_errors(this->state_); }

 protected:
  switch_::Switch *recirculation_{};
#ifdef TION_ENABLE_PRESETS
  bool enable_boost() override;
  void cancel_boost() override;
#endif

  tion4s_state_t::GatePosition get_gate_position_() const {
    if (!this->batch_active_ && this->recirculation_) {
      if (this->recirculation_->state) {
        return tion4s_state_t::GATE_POSITION_RECIRCULATION;
      }
      return tion4s_state_t::GATE_POSITION_INFLOW;
    }
    return this->state_.gate_position;
  }

  struct ControlState {
    optional<bool> power_state;
    optional<tion4s_state_t::HeaterMode> heater_mode;
    optional<uint8_t> fan_speed;
    optional<int8_t> target_temperature;
    optional<bool> buzzer;
    optional<bool> led;
    optional<tion4s_state_t::GatePosition> gate_position;
  };

  void control_state_(const ControlState &state);
};

}  // namespace tion

}  // namespace esphome
