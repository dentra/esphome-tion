#pragma once

#include "esphome/components/select/select.h"

#include "../tion-api/tion-api-3s.h"
#include "../tion/tion_controls.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

using Tion3sApi = dentra::tion::Tion3sApi;

template<class parent_t> class Tion3sAirIntakeSelect : public select::Select, public Parented<parent_t> {
 public:
  explicit Tion3sAirIntakeSelect(parent_t *parent) : Parented<parent_t>(parent) {}
  void control(const std::string &value) override {
    auto opt = this->index_of(value);
    if (opt.has_value()) {
      this->parent_->control_gate_position(static_cast<TionGatePosition>(*opt));
    }
  }
};

}  // namespace tion
}  // namespace esphome
