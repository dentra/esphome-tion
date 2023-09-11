#pragma once

#include "../tion-api/tion-api-4s.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

using TionApi4s = dentra::tion::TionApi4s;

template<class parent_t> class TionRecirculationSwitch : public Parented<parent_t>, public switch_::Switch {
 public:
  explicit TionRecirculationSwitch(parent_t *parent) : Parented<parent_t>(parent) {}
  void write_state(bool state) override { this->parent_->control_recirculation_state(state); }
};

}  // namespace tion
}  // namespace esphome
