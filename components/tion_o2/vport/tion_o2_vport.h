#pragma once

#include "esphome/core/defines.h"

#include "../../tion/tion.h"
#include "../../tion/tion_vport_uart.h"
#include "../../tion-api/tion-api-o2.h"
#include "../../tion-api/tion-api-uart-o2.h"

namespace esphome {
namespace tion {

using TionO2UartIO = TionUartIO<dentra::tion_o2::TionO2UartProtocol>;

class TionO2UartVPort : public TionVPortUARTComponent<TionO2UartIO, TionO2UartIO::frame_spec_type> {
 public:
  explicit TionO2UartVPort(TionO2UartIO *io) : TionVPortUARTComponent(io) {}

  void setup() override;
  void dump_config() override;

  void set_api(dentra::tion_o2::TionO2Api *api) { this->api_ = api; }
  void set_heartbeat_interval(uint32_t heartbeat_interval) { this->heartbeat_interval_ = heartbeat_interval; }

 protected:
  uint32_t heartbeat_interval_{};
  dentra::tion_o2::TionO2Api *api_;
};

}  // namespace tion
}  // namespace esphome
