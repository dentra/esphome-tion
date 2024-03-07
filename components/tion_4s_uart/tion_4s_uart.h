#pragma once

#include "esphome/core/defines.h"
#include "esphome/components/vport/vport_uart.h"

#include "../tion/tion_controls.h"
#include "../tion/tion_vport_uart.h"
#include "../tion-api/tion-api-4s.h"
#include "../tion-api/tion-api-uart-4s.h"

namespace esphome {
namespace tion {

using Tion4sUartIO = TionUartIO<dentra::tion::Tion4sUartProtocol>;

class Tion4sUartVPort : public TionVPortUARTComponent<Tion4sUartIO, Tion4sUartIO::frame_spec_type> {
  using TionState = dentra::tion::TionState;
  using TionGatePosition = dentra::tion::TionGatePosition;

 public:
  explicit Tion4sUartVPort(Tion4sUartIO *io) : TionVPortUARTComponent(io) {}

  void dump_config() override;
  void set_api(dentra::tion_4s::Tion4sApi *api) {}
};

}  // namespace tion
}  // namespace esphome
