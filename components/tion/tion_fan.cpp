#include "esphome/core/defines.h"
#ifdef USE_FAN
#include "esphome/core/log.h"
#include "tion_fan.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_fan";

void TionFan::control(const fan::FanCall &call) {
  bool state = this->state;
  if (call.get_state().has_value()) {
    state = *call.get_state();
    ESP_LOGD(TAG, "Set state %s", ONOFF(state));
  }

  int speed = this->speed;
  if (call.get_speed().has_value()) {
    speed = *call.get_speed();
    ESP_LOGD(TAG, "Set speed %u", speed);
  }

  this->control_fan_state(state, speed);
}

}  // namespace tion
}  // namespace esphome
#endif
