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

  void update_state() override;
  void dump_state() const override;
  void flush_state() override;

  void reset_filter() { this->api_->reset_filter(this->state_); }
};

}  // namespace tion

}  // namespace esphome
