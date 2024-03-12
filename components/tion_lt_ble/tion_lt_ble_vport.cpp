#include "esphome/core/log.h"
#include "tion_lt_ble_vport.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_lt_ble_vport";

void TionLtBleVPort::dump_config() { TION_VPORT_BLE_LOG("Tion LT BLE"); }

}  // namespace tion
}  // namespace esphome
