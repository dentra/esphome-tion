#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/application.h"

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

template<class parent_t> class TionResetFilterConfirmSwitch : public Parented<parent_t>, public switch_::Switch {
  static_assert(std::is_base_of_v<Component, parent_t>, "parent_t is not Component");

 public:
  explicit TionResetFilterConfirmSwitch(parent_t *parent) : Parented<parent_t>(parent) {}
  void write_state(bool state) override {
    constexpr const char *timeout_name = "tion_reset_confirm_timeout";
    if (state) {
      App.scheduler.set_timeout(this->parent_, timeout_name, 10000, [this]() { this->publish_state(false); });
    } else {
      App.scheduler.cancel_timeout(this->parent_, timeout_name);
    }
    this->publish_state(state);
  }
};
#endif
#ifdef USE_BUTTON
template<class parent_t> class TionResetFilterButton : public Parented<parent_t>, public button::Button {
  // static_assert(std::is_base_of<TionComponent, parent_t>::value, "parent_t is not TionComponent");

 public:
  explicit TionResetFilterButton(parent_t *parent) : Parented<parent_t>(parent) {}
  void press_action() override {
    if (this->parent_->is_reset_filter_confirmed()) {
      this->parent_->reset_filter();
    }
  }
};
#endif

}  // namespace tion
}  // namespace esphome
