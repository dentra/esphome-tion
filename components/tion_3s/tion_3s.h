#pragma once

#include "esphome/components/select/select.h"

#include "../tion-api/tion-api-3s.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

using TionApi3s = dentra::tion::TionApi3s;

template<class parent_t> class Tion3sAirIntakeSelect : public select::Select, public Parented<parent_t> {
 public:
  explicit Tion3sAirIntakeSelect(parent_t *parent) : Parented<parent_t>(parent) {}
  void control(const std::string &value) override {
    this->parent_->control_gate_position(static_cast<tion3s_state_t::GatePosition>(*this->index_of(value)));
  }
};

}  // namespace tion
}  // namespace esphome
