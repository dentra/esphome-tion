#pragma once
#include "esphome/core/defines.h"
#ifdef USE_FAN

#include "../tion_4s.h"
#include "../../tion/tion_fan_component.h"

namespace esphome {
namespace tion {

class Tion4sFan : public TionFanComponent<TionApi4s> {
 public:
  explicit Tion4sFan(TionApi4s *api, TionVPortType vport_type) : TionFanComponent(api, vport_type) {}
};

}  // namespace tion
}  // namespace esphome
#endif  // USE_FAN
