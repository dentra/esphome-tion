#include "esphome/core/defines.h"
#ifdef USE_FAN
#include "esphome/core/log.h"
#include "esphome/core/application.h"

#include "tion_fan_component.h"
#include "tion_defines.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_fan_component";

fan::FanTraits TionFanComponentBase::get_traits() {
  auto traits = fan::FanTraits(false, true, false, TION_MAX_FAN_SPEED);
#ifdef TION_ENABLE_PRESETS
  // traits.set_supported_preset_modes();
#endif
  return traits;
}

void TionFanComponentBase::control(const fan::FanCall &call) {
#ifdef TION_ENABLE_PRESETS
  // auto preset_mode = call.get_preset_mode();
  // if (!preset_mode.empty()) {
  //   auto it = this->presets_.find(preset_mode);
  //   if (it != this->presets_.end()) {
  //   }
  // }
#endif

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
#endif  // USE_FAN
