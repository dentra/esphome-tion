#pragma once

#include "esphome/components/select/select.h"

#include "../tion-api/tion-api-3s.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

using TionApi3s = dentra::tion::TionApi3s;

class Tion3s : public TionClimateComponent<TionApi3s, tion3s_state_t> {
 public:
  explicit Tion3s(TionApi3s *api) : TionClimateComponent(api) {}

  void dump_config() override;

  void set_air_intake(select::Select *air_intake) { this->air_intake_ = air_intake; }

  void update() override {
    TionClimateComponent::update();
    if (this->vport_type_ == TionVPortType::VPORT_UART && this->state_.firmware_version < 0x003C) {
      this->api_->request_after_state();
    }
  }

  void update_state() override;
  void dump_state() const override;
  void flush_state() override;

  void reset_filter() {
    this->api_->reset_filter(this->state_);
    if (this->vport_type_ == TionVPortType::VPORT_UART) {
      // allow execute command 4 for fw >= 0x003C
      this->state_.firmware_version = 0;
    }
  }

  int8_t get_unknown_temperature() const { return this->state_.unknown_temperature; }

 protected:
  select::Select *air_intake_{};
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
