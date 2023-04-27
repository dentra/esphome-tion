#pragma once

#include "../tion-api/tion-api-4s.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

using TionApi4s = dentra::tion::TionApi4s;

class Tion4s : public TionClimateComponent<TionApi4s, tion4s_state_t> {
 public:
  explicit Tion4s(TionApi4s *api) : TionClimateComponent(api) {}

  void dump_config() override;

  climate::ClimateTraits traits() override {
    auto traits = TionClimate::traits();
    traits.set_supports_action(true);
    return traits;
  }

  void set_recirculation(switch_::Switch *recirculation) { this->recirculation_ = recirculation; }

#ifdef TION_ENABLE_PRESETS
  void update() override {
    TionClimateComponent::update();
    if (this->vport_type_ == TionVPortType::VPORT_BLE) {
      this->api_->request_turbo();
    }
  }
#endif

#ifdef TION_ENABLE_SCHEDULER
  void on_ready() {
    TionClimateComponent::on_ready();

    // scheduler specific init commands
    this->api_->request_time();
    // this->api_->request_timers();
    // this->api_->request_timers_state();
  }
#endif

#ifdef TION_ENABLE_PRESETS
  void on_turbo(const tion4s_turbo_t &turbo, const uint32_t request_id);
#endif
#ifdef TION_ENABLE_SCHEDULER
  void on_time(const time_t time, const uint32_t request_id);
#endif
  void update_state() override;
  void dump_state() const override;
  void flush_state() override;

 protected:
  switch_::Switch *recirculation_{};
#ifdef TION_ENABLE_PRESETS
  bool enable_boost_() override;
  void cancel_boost_() override;
#endif
};

}  // namespace tion

}  // namespace esphome
