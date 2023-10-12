#pragma once

#include "esphome/core/defines.h"
#include "esphome/components/vport/vport_uart.h"

#include "../tion/tion.h"
#include "../tion/tion_vport_uart.h"
#include "../tion-api/tion-api-4s.h"
#include "../tion-api/tion-api-uart-4s.h"

namespace esphome {
namespace tion {

using Tion4sUartIO = TionUartIO<dentra::tion::TionUartProtocol4s>;

class Tion4sUartVPort : public TionVPortUARTComponent<Tion4sUartIO, Tion4sUartIO::frame_spec_type, PollingComponent> {
 public:
  explicit Tion4sUartVPort(Tion4sUartIO *io) : TionVPortUARTComponent(io){};

  void dump_config() override;
  void setup() override;
  void update() override { this->api_->send_heartbeat(); }

  void set_api(dentra::tion::TionApi4s *api) { this->api_ = api; }
  void set_heartbeat_interval(uint32_t heartbeat_interval) { this->heartbeat_interval_ = heartbeat_interval; }

 protected:
  uint32_t heartbeat_interval_{5000};
  dentra::tion::TionApi4s *api_;
};

}  // namespace tion
}  // namespace esphome
