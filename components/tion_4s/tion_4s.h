#pragma once

#include "../tion-api/tion-api-4s.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

enum DirtyState : uint8_t {
  DIRTY_STATE = 1 << 0,
  DIRTY_BOOST_ENABLE = 1 << 1,
  DIRTY_BOOST_CANCEL = 1 << 2,
};

class Tion4s : public TionClimateComponent, public Tion<TionApi4s>, public TionDisconnectMixin<Tion4s> {
 public:
  climate::ClimateTraits traits() override {
    auto traits = TionClimate::traits();
    traits.set_supports_action(true);
    return traits;
  }

  void update() override;
  void on_ready() override;
  bool write_state() override;

  void set_recirculation(switch_::Switch *recirculation) { this->recirculation_ = recirculation; }

  void read(const tion_dev_status_t &status) override;
  void read(const tion4s_state_t &state) override;
  void read(const tion4s_turbo_t &turbo) override;
  void read(const tion4s_time_t &time) override;

  void run_polling();

 protected:
  switch_::Switch *recirculation_{};
  uint8_t dirty_flag_{};

  void flush_state_(const tion4s_state_t &state) const;

  bool enable_boost_() override;
  void cancel_boost_() override;

  bool is_dirty_(DirtyState flag) { return (this->dirty_flag_ & flag) != 0; }
  void set_dirty_(DirtyState flag) { this->dirty_flag_ |= flag; }
  void drop_dirty_(DirtyState flag) { this->dirty_flag_ &= ~flag; }
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
