#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"

#include "esphome/components/button/button.h"
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif

#include "../tion_api_component.h"
#include "../tion_properties.h"

namespace esphome {
namespace tion {

// C - PropertyController
template<class C> class TionButton : public button::Button, public Component, public Parented<TionApiComponent> {
 protected:
  using TionState = dentra::tion::TionState;
  using PC = property_controller::Controller<C>;

  constexpr static const auto *TAG = "tion_button";

 public:
  explicit TionButton(TionApiComponent *api) : Parented(api) {}

  float get_setup_priority() const override { return setup_priority::LATE; }

  void dump_config() override { LOG_BUTTON("", "Tion Button", this); }

  void setup() override {
    if (!C::is_supported(this->parent_)) {
      PC::mark_unsupported(this);
      return;
    }
  }

 protected:
  void press_action() override { C::press_action(this->parent_); }
};

class TionResetFilterButton2 : public TionButton<property_controller::button::ResetFilter> {
 public:
  explicit TionResetFilterButton2(TionApiComponent *api) : TionButton(api) {}
#ifdef USE_SWITCH

  void setup() override {
    TionButton::setup();
    if (this->is_failed()) {
      if (this->confirm_) {
        PC::mark_unsupported_entity(this->confirm_);
      }
      return;
    }
  }

  void set_confirm(switch_::Switch *confirm) { this->confirm_ = confirm; }

 protected:
  switch_::Switch *confirm_{};

  void press_action() override {
    if (this->confirm_ == nullptr || this->confirm_->state) {
      TionButton::press_action();
    }
  }
#endif
};

}  // namespace tion
}  // namespace esphome
