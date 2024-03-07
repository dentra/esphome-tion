#pragma once

#include "esphome/core/defines.h"

#include "../tion-api/tion-api-lt.h"
#include "../tion/tion_api_component.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

// class TionLtBaseComponent {
//  public:
//   TionLtBaseComponent(TionLtApi *api) {}

//   void control_state(bool power_state, bool heater_state, uint8_t fan_speed, int8_t target_temperature, bool buzzer,
//                      bool led);

//  protected:
//   uint32_t request_id_{};
// };

class TionLtApiComponent : public TionApiComponentBase<TionLtApi> {
 public:
  explicit TionLtApiComponent(TionLtApi *api, TionVPortType vport_type) : TionApiComponentBase(api, vport_type) {}

  void set_button_presets(const dentra::tion_lt::button_presets_t &button_presets) {
    this->typed_api()->set_button_presets(button_presets);
  }
};

}  // namespace tion
}  // namespace esphome
