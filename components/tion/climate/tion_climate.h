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
  void setup() override;

  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  void set_enable_heat_cool(bool enable_heat_cool) { this->enable_heat_cool_ = enable_heat_cool; }
  void set_enable_fan_auto(bool enable_fan_auto) { this->enable_fan_auto_ = enable_fan_auto; }

 protected:
  bool enable_heat_cool_{};
  bool enable_fan_auto_{};
  void on_state_(const TionState &state);
  bool set_fan_speed_(uint8_t fan_speed);
};

}  // namespace tion
}  // namespace esphome
