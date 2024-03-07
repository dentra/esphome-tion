#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"

#include "esphome/components/binary_sensor/binary_sensor.h"

#include "../tion_api_component.h"
#include "../tion_properties.h"

namespace esphome {
namespace tion {

// template<typename> constexpr std::false_type has_is_supported(long);
// template<typename T> constexpr auto has_is_supported(int) -> decltype(T::is_supported(nullptr), std::true_type{});
// template<typename T> using has_is_supported = decltype(has_is_supported<T>(0));

// C - PropertyController
template<class C>
class TionBinarySensor : public binary_sensor::BinarySensor, public Component, public Parented<TionApiComponent> {
  using TionState = dentra::tion::TionState;
  using PC = property_controller::Controller<C>;

  constexpr static const auto *TAG = "tion_binary_sensor";

 public:
  explicit TionBinarySensor(TionApiComponent *api) : Parented(api) {}

  float get_setup_priority() const override { return setup_priority::LATE; }

  void dump_config() override { LOG_BINARY_SENSOR("", "Tion BinarySensor", this); }

  void setup() override {
    if (!PC::is_supported(this)) {
      return;
    }
    this->parent_->add_on_state_callback([this](const TionState *state) {
      if (!PC::publish_state(this, state)) {
        this->has_state_ = false;
        this->state_callback_.call(false);
      }
    });
  }
};

}  // namespace tion
}  // namespace esphome
