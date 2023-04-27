#pragma once

#include "../tion-api/tion-api-lt.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

class TionLt : public TionClimateComponent<TionApiLt, tionlt_state_t> {
 public:
  explicit TionLt(TionApiLt *api) : TionClimateComponent(api) {}

  void dump_config() override;

  climate::ClimateTraits traits() override {
    auto traits = TionClimate::traits();
    traits.set_supports_action(true);
    return traits;
  }

  void update_state() override;
  void dump_state() const override;
  void flush_state() override;
};

}  // namespace tion

}  // namespace esphome
