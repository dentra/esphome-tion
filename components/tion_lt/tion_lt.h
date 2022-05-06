#pragma once

#include "../tion-api/tion-api-lt.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

class TionLt : public TionClimateComponentWithBoost, public Tion<TionApiLt> {
 public:
  climate::ClimateTraits traits() override {
    auto traits = TionClimate::traits();
    traits.set_supports_action(true);
    return traits;
  }

  void update() override { this->parent_->set_enabled(true); }

  void on_ready() override;
  bool write_state() override {
    this->publish_state();
    this->dirty_ = true;
    this->parent_->set_enabled(true);
    return true;
  };

  void read(const tion_dev_status_t &status) override;
  void read(const tionlt_state_t &state) override;

 protected:
  bool dirty_{};
  void update_state_(tionlt_state_t &state);
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
