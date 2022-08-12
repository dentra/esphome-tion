#pragma once

#include "../tion-api/tion-api-4s.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

class Tion4s : public TionClimateComponent<TionApi4s, tion4s_state_t> {
 public:
  explicit Tion4s(TionVPort *vport) : TionClimateComponent(vport) {}

  void dump_config() override;

  climate::ClimateTraits traits() override {
    auto traits = TionClimate::traits();
    traits.set_supports_action(true);
    return traits;
  }

  void set_recirculation(switch_::Switch *recirculation) { this->recirculation_ = recirculation; }

  bool on_ready() override;
  bool on_poll() const override;

  void on_turbo(const tion4s_turbo_t &turbo, const uint32_t request_id) override;
  void on_time(const time_t time, const uint32_t request_id) override;

  void update_state(const tion4s_state_t &state) override;
  void flush_state(const tion4s_state_t &state) const override;
  void dump_state(const tion4s_state_t &state) const override;

 protected:
  switch_::Switch *recirculation_{};

  bool enable_boost_() override;
  void cancel_boost_() override;
};

}  // namespace tion

}  // namespace esphome
