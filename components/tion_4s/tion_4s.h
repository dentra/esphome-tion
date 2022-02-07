#pragma once

#include "esphome/components/number/number.h"
#include "esphome/components/time/real_time_clock.h"

#include "../tion-api/tion-api-4s.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

enum UpdateState : uint8_t {
  UPDATE_STATE = 1 << 0,
  UPDATE_BOOST = 1 << 1,
};

class Tion4s : public TionComponent, public TionClimate, public Tion<TionApi4s> {
 public:
  void update() override { this->parent_->set_enabled(true); }

  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  void on_ready() override;

  void set_recirculation(switch_::Switch *recirculation) { this->recirculation_ = recirculation; }
  void set_boost_time(number::Number *boost_time) { this->boost_time_ = boost_time; }

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
  number::Number *boost_time_{};
  void update_state_(tion4s_state_t &state) const;
  uint32_t update_flag_{};
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

class Tion4sBoostTimeNumber : public number::Number, public Parented<Tion4s> {
 public:
 protected:
  virtual void control(float value) { this->publish_state(value); }
};

}  // namespace tion

}  // namespace esphome
