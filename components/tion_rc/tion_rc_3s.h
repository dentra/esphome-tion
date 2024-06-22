#pragma once

#include "tion_rc.h"

#include "../tion-api/tion-api-ble-3s.h"

namespace esphome {
namespace tion_rc {

class Tion3sRC : public TionRCControlImpl<dentra::tion::Tion3sBleProtocol> {
 public:
  Tion3sRC(dentra::tion::TionApiBase *api) : TionRCControlImpl(api) {}
  void adv(bool pair);
  void on_state(const dentra::tion::TionState &st);
  void on_frame(uint16_t type, const uint8_t *data, size_t size);
};

}  // namespace tion_rc
}  // namespace esphome
