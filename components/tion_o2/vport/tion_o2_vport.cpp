#include "esphome/core/log.h"
#include "esphome/core/defines.h"
#ifdef USE_OTA
#include "esphome/components/ota/ota_component.h"
#endif

#include "tion_o2_vport.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_o2_uart";

void TionO2UartVPort::dump_config() { VPORT_UART_LOG("Tion O2 UART"); }

}  // namespace tion
}  // namespace esphome
