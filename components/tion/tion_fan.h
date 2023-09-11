#pragma once

#include "esphome/core/defines.h"

#ifdef USE_FAN
#include "esphome/components/fan/fan.h"

namespace esphome {
namespace tion {

class TionFan : public fan::Fan {
 public:
  fan::FanTraits get_traits() override { return fan::FanTraits(false, true, false, 6); }

  virtual void control_fan_state(bool state, uint8_t speed) = 0;

 protected:
  void control(const fan::FanCall &call) override;
};

}  // namespace tion
}  // namespace esphome
#endif
