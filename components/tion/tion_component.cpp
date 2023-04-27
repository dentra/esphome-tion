#include "esphome/core/log.h"

#include "tion_climate.h"
#include "tion_component.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_component";

void TionComponent::setup() {
#ifdef TION_ENABLE_PRESETS
  if (this->boost_time_) {
    this->rtc_ = global_preferences->make_preference<uint8_t>(fnv1_hash(TAG));
    uint8_t boost_time;
    if (!this->rtc_.load(&boost_time)) {
      boost_time = DEFAULT_BOOST_TIME_SEC / 60;
    }
    auto call = this->boost_time_->make_call();
    call.set_value(boost_time);
    call.perform();
  }
#endif
}

void TionComponent::update_dev_status_(const dentra::tion::tion_dev_status_t &status) {
  if (this->version_ != nullptr) {
    this->version_->publish_state(str_snprintf("%04X", 4, status.firmware_version));
  }
#if TION_LOG_LEVEL >= TION_LOG_LEVEL_VERBOSE
  ESP_LOGV(TAG, "Work Mode       : %02X", status.work_mode);
  ESP_LOGV(TAG, "Device type     : %04X", status.device_type);
  ESP_LOGV(TAG, "Hardware version: %04X", status.hardware_version);
  ESP_LOGV(TAG, "Firmware version: %04X", status.firmware_version);
#endif
}

}  // namespace tion
}  // namespace esphome
