#pragma once
#if 0

#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"

#include "esphome/components/switch/switch.h"

// #include "esphome/components/number/number.h"
// #include "esphome/components/sensor/sensor.h"

#include "../tion_api_component.h"

namespace esphome {
namespace tion {

class TionBoost : public switch_::Switch, public Component, public Parented<TionApiComponent> {
  using TionState = dentra::tion::TionState;
  using TionGatePosition = dentra::tion::TionGatePosition;

 public:
  explicit TionBoost(TionApiComponent *api) : Parented(api) {}

  void dump_config() override;
  void setup() override;

  void boost_enable();
  void boost_cancel();

  struct BoostPresetData {
    int8_t target_temperature;
    bool heater_state;
  };

  struct PresetData : public BoostPresetData {
    bool power_state;
    uint8_t fan_speed;
    TionGatePosition gate_position;
  };

  bool assumed_state() override { return this->is_failed(); }

  void set_update_interval(uint32_t update_interval) { this->update_interval_ = update_interval; }
  uint32_t get_update_interval() const { return this->update_interval_; }

 protected:
  uint32_t update_interval_{};
  uint8_t boost_time_{};
  BoostPresetData boost_preset_{};
  PresetData boost_save_{};

  void write_state(bool state) override;
  void on_state_(const TionState &state);
};

}  // namespace tion
}  // namespace esphome
#endif
