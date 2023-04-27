#include "esphome/core/log.h"

#include "tion_3s_uart.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_3s_uart";

void Tion3sUartVPort::dump_config() {
  VPORT_UART_LOG("Tion 3S UART");
}

}  // namespace tion
}  // namespace esphome
