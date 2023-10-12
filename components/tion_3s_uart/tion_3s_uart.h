#pragma once

#include "esphome/core/defines.h"

#include "../tion/tion.h"
#include "../tion/tion_vport_uart.h"
#include "../tion-api/tion-api-3s.h"
#include "../tion-api/tion-api-uart-3s.h"

namespace esphome {
namespace tion {

using Tion3sUartIO = TionUartIO<dentra::tion::Tion3sUartProtocol>;

class Tion3sUartVPort : public TionVPortUARTComponent<Tion3sUartIO, Tion3sUartIO::frame_spec_type, PollingComponent> {
 public:
  explicit Tion3sUartVPort(Tion3sUartIO *io) : TionVPortUARTComponent(io) {}

  void dump_config() override;
  void update() override { this->api_->request_command4(); }

  void set_api(dentra::tion::Tion3sApi *api) { this->api_ = api; }

 protected:
  dentra::tion::Tion3sApi *api_;
};

}  // namespace tion
}  // namespace esphome
