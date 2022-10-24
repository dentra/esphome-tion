#pragma once

#include "../tion-api/tion-api-4s.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

using TionApi4s = dentra::tion::TionApi4s;

class Tion4s final : public TionClimateComponent<TionApi4s, tion4s_state_t> {
 public:
  explicit Tion4s(TionApi4s *api, vport::VPortComponent<uint16_t> *vport) : TionClimateComponent(api, vport) {
    vport->on_ready.set<Tion4s, &Tion4s::on_ready>(*this);
    vport->on_update.set<Tion4s, &Tion4s::on_update>(*this);
  }

  void dump_config() override;

  climate::ClimateTraits traits() override {
    auto traits = TionClimate::traits();
    traits.set_supports_action(true);
    return traits;
  }

  void set_recirculation(switch_::Switch *recirculation) { this->recirculation_ = recirculation; }

  bool on_ready();
  bool on_update();
#ifdef TION_ENABLE_PRESETS
  void on_turbo(const tion4s_turbo_t &turbo, const uint32_t request_id);
#endif
#ifdef TION_ENABLE_SCHEDULER
  void on_time(const time_t time, const uint32_t request_id);
#endif
  void update_state(const tion4s_state_t &state) override;
  void flush_state(const tion4s_state_t &state) const override;
  void dump_state(const tion4s_state_t &state) const override;

 protected:
  switch_::Switch *recirculation_{};
#ifdef TION_ENABLE_PRESETS
  bool enable_boost_() override;
  void cancel_boost_() override;
#endif
};

}  // namespace tion

}  // namespace esphome
