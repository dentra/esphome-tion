#include "esphome/core/log.h"
#include "tion_ble.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_ble";

void TionBLEVPortBase::dump_config() { this->dump_settings(TAG); }

void TionBLEVPortBase::dump_settings(const char *TAG) {
  VPORT_BLE_LOG(TAG, "Tion BLE");
  ESP_LOGCONFIG(TAG, "  State timeout: %s",
                this->state_timeout_ > 0 ? (to_string(this->state_timeout_ / 1000) + "s").c_str() : ONOFF(false));
}

void TionBLEVPortBase::update() {
  if (this->is_connected()) {
    this->fire_poll();
    if (!this->is_persistent_connection()) {
      this->schedule_disconnect(this->state_timeout_);
    }
  } else {
    this->connect();
  }
}

}  // namespace tion
}  // namespace esphome
