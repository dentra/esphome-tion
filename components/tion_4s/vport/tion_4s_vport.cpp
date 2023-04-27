#include "esphome/core/log.h"
#include "tion_4s_vport.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_4s_vport";

void Tion4sBleVPort::dump_config() { TION_VPORT_BLE_LOG("Tion 4S BLE"); }

}  // namespace tion
}  // namespace esphome
