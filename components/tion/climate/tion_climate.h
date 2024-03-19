#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"

#include "esphome/components/climate/climate.h"

#include "../tion_component.h"

namespace esphome {
namespace tion {

class TionClimate : public climate::Climate, public Component, public Parented<TionApiComponent> {
  using TionState = dentra::tion::TionState;

 public:
  explicit TionClimate(TionApiComponent *api) : Parented(api) {}

  float get_setup_priority() const override { return setup_priority::LATE; }

  void dump_config() override;

  void setup() override {
    this->parent_->add_on_state_callback([this](const TionState *state) {
      if (state) {
        this->on_state_(*state);
      }
    });
  }

  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  void set_enable_heat_cool(bool enable_heat_cool) { this->enable_heat_cool_ = enable_heat_cool; }

 protected:
  bool enable_heat_cool_{};
  void on_state_(const TionState &state);
  bool set_fan_speed_(uint8_t fan_speed);
};

}  // namespace tion
}  // namespace esphome
