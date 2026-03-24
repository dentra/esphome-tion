#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/version.h"

#include "esphome/components/binary_sensor/binary_sensor.h"

#include "../tion_component.h"
#include "../tion_properties.h"
#include "../tion_extension.h"

namespace esphome {
namespace tion {

// C - PropertyController
template<class C>
class TionBinarySensor : public EntityExtension<binary_sensor::BinarySensor>,
                         public Component,
                         public Parented<TionApiComponent> {
  using TionState = dentra::tion::TionState;
  using PC = property_controller::Controller<C>;

  constexpr static const auto *TAG = "tion_binary_sensor";

 public:
  explicit TionBinarySensor(TionApiComponent *api) : Parented(api) {}

  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void dump_config() override {
    if (this->is_failed()) {
      return;
    }
    LOG_BINARY_SENSOR("", "Tion BinarySensor", this);
  }

  void setup() override {
    ESP_LOGD(TAG, "Setting up %s...", this->get_name().c_str());
    if (!PC::is_supported(this)) {
      return;
    }
    this->parent_->add_on_state_callback([this](const TionState *state) {
      if (!PC::publish_state(this, state)) {
        this->set_new_state({});
        this->set_has_state(false);
      }
    });
  }
};

}  // namespace tion
}  // namespace esphome
