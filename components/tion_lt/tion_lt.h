#pragma once

#include "esphome/core/defines.h"
#include "../tion-api/tion-api-lt.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

class TionLtBaseComponent {
 public:
  TionLtBaseComponent(TionLtApi *api) {}

  void control_state(bool power_state, bool heater_state, uint8_t fan_speed, int8_t target_temperature, bool buzzer,
                     bool led);

 protected:
  uint32_t request_id_{};
};

}  // namespace tion
}  // namespace esphome
