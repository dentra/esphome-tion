#include "esphome/core/log.h"

#include "tion_auto_switch.h"

namespace esphome {
namespace tion {

void TionAutoSwitch::update_co2_(float value) {
  if (!this->state || property_controller::binary_sensor::Boost::get(this->parent_->state())) {
    return;
  }
  auto *call = this->parent_->make_call();
  if (this->api()->update_auto(value, call)) {
    call->perform();
  }
}

}  // namespace tion
}  // namespace esphome
