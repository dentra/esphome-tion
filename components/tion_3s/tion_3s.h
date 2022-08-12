#pragma once

#include "esphome/components/select/select.h"

#include "../tion-api/tion-api-3s.h"
#include "../tion/tion.h"
#include "../tion_ble/tion_ble.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

class Tion3s : public TionClimateComponent<TionsApi3s, tion3s_state_t> {
 public:
  explicit Tion3s(TionVPort *vport) : TionClimateComponent(vport) {}

  void dump_config() override;

  void set_air_intake(select::Select *air_intake) { this->air_intake_ = air_intake; }

  void update_state(const tion3s_state_t &state) override;
  void flush_state(const tion3s_state_t &state) const;
  void dump_state(const tion3s_state_t &state) const override;

 protected:
  select::Select *air_intake_{};
  uint8_t saved_fan_speed_{};
};

class Tion3sAirIntakeSelect : public select::Select {
 public:
  explicit Tion3sAirIntakeSelect(Tion3s *parent) : parent_(parent) {}
  void control(const std::string &value) override {
    this->publish_state(value);
    this->parent_->write_climate_state();
  }

 protected:
  Tion3s *parent_;
};

}  // namespace tion
}  // namespace esphome
