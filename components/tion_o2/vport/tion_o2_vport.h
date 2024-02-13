#pragma once

#include "esphome/core/defines.h"

#include "../../tion/tion_controls.h"
#include "../../tion/tion_vport_uart.h"
#include "../../tion-api/tion-api-o2.h"
#include "../../tion-api/tion-api-uart-o2.h"

namespace esphome {
namespace tion {

using TionO2UartIO = TionUartIO<dentra::tion_o2::TionO2UartProtocol>;

class TionO2UartVPort : public TionVPortUARTComponent<TionO2UartIO, TionO2UartIO::frame_spec_type> {
 public:
  explicit TionO2UartVPort(TionO2UartIO *io) : TionVPortUARTComponent(io) {}

  void dump_config() override;

  void set_api(dentra::tion_o2::TionO2Api *api) {}
};

}  // namespace tion
}  // namespace esphome
