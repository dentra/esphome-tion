#pragma once

#include "tion_rc.h"
#include "../tion-api/tion-api-ble-lt.h"

namespace esphome {
namespace tion_rc {

class Tion4sRCBleProtocol : public dentra::tion::TionLtBleProtocol {
 public:
  Tion4sRCBleProtocol() : dentra::tion::TionLtBleProtocol(false) {}
};

class Tion4sRC : public TionRCControlImpl<Tion4sRCBleProtocol> {
 public:
  Tion4sRC(dentra::tion::TionApiBase *api) : TionRCControlImpl(api) {}
  void adv(bool pair) override;
  void on_state(const dentra::tion::TionState &st) override;
  void on_frame(uint16_t type, const uint8_t *data, size_t size) override;
};

}  // namespace tion_rc
}  // namespace esphome
