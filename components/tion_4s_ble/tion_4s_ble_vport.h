#pragma once

#include "../tion-api/tion-api-4s.h"
#include "../tion-api/tion-api-ble-lt.h"
#include "../tion/tion_vport_ble.h"

namespace esphome {
namespace tion {

using Tion4sBleIO = esphome::tion::TionBleIO<dentra::tion::TionLtBleProtocol>;

class Tion4sBleVPort : public TionVPortBLEComponent<Tion4sBleIO> {
 public:
  Tion4sBleVPort(io_type *io) : TionVPortBLEComponent(io) {}

  void dump_config() override;

  void set_api(void *) {}
};

}  // namespace tion
}  // namespace esphome
