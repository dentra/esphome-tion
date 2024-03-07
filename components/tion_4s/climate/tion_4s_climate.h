#pragma once
#include "esphome/core/defines.h"
// #ifdef USE_TION_CLIMATE

#include "esphome/components/switch/switch.h"

#include "../tion_4s.h"
#include "../../tion/tion_climate_component.h"

namespace esphome {
namespace tion {

using Tion4sClimateBase = TionLtClimateComponent<Tion4sApi>;

class Tion4sClimate : public Tion4sClimateBase {
 public:
  explicit Tion4sClimate(Tion4sApi *api, TionVPortType vport_type) : Tion4sClimateBase(api, vport_type) {
#ifdef TION_ENABLE_SCHEDULER
    this->api_->on_time.set<Tion4sClimate, &Tion4sClimate::on_time>(*this);
    this->api_->on_timer.set<Tion4sClimate, &Tion4sClimate::on_timer>(*this);
    this->api_->on_timers_state.set<Tion4sClimate, &Tion4sClimate::on_timers_state>(*this);
#endif
  }

  void dump_config() override;
  void setup() override;

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
  void on_turbo(const dentra::tion_4s::tion4s_turbo_t &turbo, uint32_t request_id);
#endif
#ifdef TION_ENABLE_SCHEDULER
  void on_time(time_t time, uint32_t request_id);
  void on_timer(uint8_t timer_id, const dentra::tion_4s::tion4s_timer_t &timer, uint32_t request_id);
  void on_timers_state(const dentra::tion_4s::tion4s_timers_state_t &timers_state, uint32_t request_id);
  void dump_timers();
  void reset_timers();
#endif
  void update_state(const tion::TionState &state) override;

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

  void control_recirculation_state(bool state);
  void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, float target_temperature,
                             TionGatePosition gate_position) override;

  TionGatePosition get_gate_position() const override {
    if (!this->batch_active_ && this->recirculation_) {
      if (this->recirculation_->state) {
        return TionGatePosition::INDOOR;
      }
      return TionGatePosition::OUTDOOR;
    }
    return this->state_.gate_position;
  }

  void reset_errors() const { this->api_->reset_errors(this->state_); }

  void set_heartbeat_interval(uint32_t heartbeat_interval) {
#ifdef TION_ENABLE_HEARTBEAT
    this->heartbeat_interval_ = heartbeat_interval;
#endif
  }

 protected:
  switch_::Switch *recirculation_{};
#ifdef TION_ENABLE_PRESETS
  bool enable_boost() override;
  void cancel_boost() override;
#endif

#ifdef TION_ENABLE_HEARTBEAT
  uint32_t heartbeat_interval_{5000};
#endif
};

}  // namespace tion

}  // namespace esphome
// #endif  // USE_TION_CLIMATE
