#pragma once

#include "../tion-api/tion-api-lt.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

class TionLt : public TionClimateComponent<TionApiLt, tionlt_state_t> {
 public:
  explicit TionLt(TionApiLt *api, vport::VPortComponent<uint16_t> *vport) : TionClimateComponent(api, vport) {}

  void dump_config() override;

  climate::ClimateTraits traits() override {
    auto traits = TionClimate::traits();
    traits.set_supports_action(true);
    return traits;
  }

  void update_state(const tionlt_state_t &state) override;
  void flush_state(const tionlt_state_t &state) const;
  void dump_state(const tionlt_state_t &state) const override;
};

}  // namespace tion

}  // namespace esphome
