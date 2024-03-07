#include "esphome/core/log.h"
#include "esphome/core/defines.h"
#ifdef USE_OTA
#include "esphome/components/ota/ota_component.h"
#endif

#include "tion_4s_uart.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_4s_uart";

void Tion4sUartVPort::dump_config() { VPORT_UART_LOG("Tion 4S UART"); }

}  // namespace tion
}  // namespace esphome
