#pragma once

#include "../tion-api/tion-api-4s.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

enum UpdateState : uint8_t {
  UPDATE_STATE = 1 << 0,
  UPDATE_BOOST_ENABLE = 1 << 1,
  UPDATE_BOOST_CANCEL = 1 << 2,
};

class Tion4s : public TionClimateComponent, public Tion<TionApi4s> {
 public:
  climate::ClimateTraits traits() override {
    auto traits = TionClimate::traits();
    traits.set_supports_action(true);
    return traits;
  }

  void update() override { this->parent_->set_enabled(true); }

  void on_ready() override;

  void set_recirculation(switch_::Switch *recirculation) { this->recirculation_ = recirculation; }

  bool write_state() override {
    this->publish_state();
    this->update_flag_ |= UPDATE_STATE;
    this->parent_->set_enabled(true);
    return true;
  };

  void read(const tion_dev_status_t &status) override;
  void read(const tion4s_state_t &state) override;
  void read(const tion4s_turbo_t &turbo) override;
  void read(const tion4s_time_t &time) override;

 protected:
  switch_::Switch *recirculation_{};

  void update_state_(tion4s_state_t &state) const;
  uint32_t update_flag_{};

  bool enable_boost_() override;
  void cancel_boost_() override;
};

class Tion4sLedSwitch : public switch_::Switch {
 public:
  explicit Tion4sLedSwitch(Tion4s *parent) : parent_(parent) {}
  void write_state(bool state) override {
    this->publish_state(state);
    this->parent_->write_state();
  }

 protected:
  Tion4s *parent_;
};

class Tion4sBuzzerSwitch : public switch_::Switch {
 public:
  explicit Tion4sBuzzerSwitch(Tion4s *parent) : parent_(parent) {}
  void write_state(bool state) override {
    this->publish_state(state);
    this->parent_->write_state();
  }

 protected:
  Tion4s *parent_;
};

class Tion4sRecirculationSwitch : public switch_::Switch {
 public:
  explicit Tion4sRecirculationSwitch(Tion4s *parent) : parent_(parent) {}
  void write_state(bool state) override {
    this->publish_state(state);
    this->parent_->write_state();
  }

 protected:
  Tion4s *parent_;
};

}  // namespace tion

}  // namespace esphome
