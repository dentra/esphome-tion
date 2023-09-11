#pragma once

#pragma once

#include "../tion_4s.h"
#include "../../tion/tion_fan_component.h"

namespace esphome {
namespace tion {

class Tion4sFan : public TionFanComponent<TionApi4s> {
 public:
  explicit Tion4sFan(TionApi4s *api) {}
};

}  // namespace tion
}  // namespace esphome
