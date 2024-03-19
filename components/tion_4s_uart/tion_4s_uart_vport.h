#pragma once

#include "esphome/core/defines.h"
#include "esphome/components/vport/vport_uart.h"

#include "../tion/tion_vport_uart.h"
#include "../tion-api/tion-api-4s.h"
#include "../tion-api/tion-api-uart-4s.h"

namespace esphome {
namespace tion {

using Tion4sUartIO = TionUartIO<dentra::tion::Tion4sUartProtocol>;

class Tion4sUartVPort : public TionVPortUARTComponent<Tion4sUartIO> {
 public:
  explicit Tion4sUartVPort(io_type *io) : TionVPortUARTComponent(io) {}

  void dump_config() override;
  void setup() override;

  void set_api(dentra::tion_4s::Tion4sApi *api) { this->api_ = api; }
  void set_heartbeat_interval(uint32_t heartbeat_interval) { this->heartbeat_interval_ = heartbeat_interval; }

 protected:
  uint32_t heartbeat_interval_{5000};
  dentra::tion_4s::Tion4sApi *api_;
};

}  // namespace tion
}  // namespace esphome
