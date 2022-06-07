#pragma once

#include "../tion-api/tion-api-lt.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

class TionLt : public TionClimateComponentWithBoost, public Tion<TionApiLt>, public TionDisconnectMixin<TionLt> {
 public:
  climate::ClimateTraits traits() override {
    auto traits = TionClimate::traits();
    traits.set_supports_action(true);
    return traits;
  }

  void update() override;
  void on_ready() override;
  bool write_state() override;

  void read(const tion_dev_status_t &status) override;
  void read(const tionlt_state_t &state) override;

  void run_polling();

 protected:
  bool dirty_{};
  void flush_state_(const tionlt_state_t &state) const;
};

class TionLtLedSwitch : public switch_::Switch {
 public:
  explicit TionLtLedSwitch(TionLt *parent) : parent_(parent) {}
  void write_state(bool state) override {
    this->publish_state(state);
    this->parent_->write_state();
  }

 protected:
  TionLt *parent_;
};

class TionLtBuzzerSwitch : public switch_::Switch {
 public:
  explicit TionLtBuzzerSwitch(TionLt *parent) : parent_(parent) {}
  void write_state(bool state) override {
    this->publish_state(state);
    this->parent_->write_state();
  }

 protected:
  TionLt *parent_;
};

}  // namespace tion

}  // namespace esphome
