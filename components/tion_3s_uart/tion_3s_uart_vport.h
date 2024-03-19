#pragma once

#include "esphome/core/defines.h"

#include "../tion/tion_vport_uart.h"
#include "../tion-api/tion-api-3s.h"
#include "../tion-api/tion-api-uart-3s.h"

namespace esphome {
namespace tion {

using Tion3sUartIO = TionUartIO<dentra::tion::Tion3sUartProtocol>;

class Tion3sUartVPort : public TionVPortUARTComponent<Tion3sUartIO, PollingComponent> {
 public:
  explicit Tion3sUartVPort(io_type *io) : TionVPortUARTComponent(io) {}

  void dump_config() override;
  // TODO нужно ли делать этот запрос
  void update() override { this->api_->request_command4(); }

  void set_api(dentra::tion::Tion3sApi *api) { this->api_ = api; }

 protected:
  dentra::tion::Tion3sApi *api_;
};

}  // namespace tion
}  // namespace esphome
