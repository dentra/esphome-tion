#include <cinttypes>

#include "esphome/core/log.h"

#include "tion_component.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_component";

void TionComponent::update_dev_info_(const dentra::tion::tion_dev_info_t &info) {
#ifdef USE_TION_VERSION
  if (this->version_ != nullptr) {
    this->version_->publish_state(str_snprintf("%04X", 4, info.firmware_version));
  }
#endif
#if TION_LOG_LEVEL >= TION_LOG_LEVEL_VERBOSE
  const char *work_mode_str;
  if (info.work_mode == dentra::tion::tion_dev_info_t::NORMAL) {
    work_mode_str = "NORMAL";
  } else if (info.work_mode == dentra::tion::tion_dev_info_t::UPDATE) {
    work_mode_str = "UPDATE";
  } else {
    work_mode_str = "UNKNOWN";
  }
  ESP_LOGV(TAG, "Work Mode       : %d (%s)", info.work_mode, work_mode_str);
  ESP_LOGV(TAG, "Device type     : %08" PRIX32, info.device_type);
  ESP_LOGV(TAG, "Hardware version: %04X", info.hardware_version);
  ESP_LOGV(TAG, "Firmware version: %04X", info.firmware_version);
#endif
}

}  // namespace tion
}  // namespace esphome
