#pragma once

#include "esphome/core/defines.h"
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif

namespace esphome {
namespace tion {

#ifdef USE_SWITCH
template<class parent_t> class TionBuzzerSwitch : public Parented<parent_t>, public switch_::Switch {
 public:
  explicit TionBuzzerSwitch(parent_t *parent) : Parented<parent_t>(parent) {}
  void write_state(bool state) override { this->parent_->control_buzzer_state(state); }
};

template<class parent_t> class TionLedSwitch : public Parented<parent_t>, public switch_::Switch {
 public:
  explicit TionLedSwitch(parent_t *parent) : Parented<parent_t>(parent) {}
  void write_state(bool state) override { this->parent_->control_led_state(state); }
};
#endif
#ifdef USE_BUTTON
template<class parent_t> class TionResetFilterButton : public Parented<parent_t>, public button::Button {
 public:
  explicit TionResetFilterButton(parent_t *parent) : Parented<parent_t>(parent) {}
  void press_action() override { this->parent_->reset_filter(); }
};
#endif

}  // namespace tion
}  // namespace esphome
