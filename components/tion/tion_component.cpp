#include "esphome/core/log.h"

#include "tion_climate.h"
#include "tion_component.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_component";

void TionComponent::call_setup() {
#ifdef TION_ENABLE_PRESETS
  if (this->boost_time_) {
    this->boost_rtc_ = global_preferences->make_preference<uint8_t>(fnv1_hash(TAG));
    uint8_t boost_time;
    if (!this->boost_rtc_.load(&boost_time)) {
      boost_time = DEFAULT_BOOST_TIME_SEC / 60;
    }
    auto call = this->boost_time_->make_call();
    call.set_value(boost_time);
    call.perform();
  }
#endif
}

void TionComponent::update_dev_info_(const dentra::tion::tion_dev_info_t &info) {
  if (this->version_ != nullptr) {
    this->version_->publish_state(str_snprintf("%04X", 4, info.firmware_version));
  }
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
  ESP_LOGV(TAG, "Device type     : %08X", info.device_type);
  ESP_LOGV(TAG, "Hardware version: %04X", info.hardware_version);
  ESP_LOGV(TAG, "Firmware version: %04X", info.firmware_version);
#endif
}

}  // namespace tion
}  // namespace esphome
