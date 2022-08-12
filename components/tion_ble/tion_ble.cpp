#include "esphome/core/log.h"
#include "tion_ble.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_ble";

void TionBLEVPort::dump_component_config(const char *TAG) const {
  ESP_LOGCONFIG(TAG, "Virtual BLE port:");
  ESP_LOGCONFIG(TAG, "  Update interval: %us", this->update_interval_ / 1000);
  ESP_LOGCONFIG(TAG, "  State timeout: %s",
                this->state_timeout_ > 0 ? (to_string(this->state_timeout_ / 1000) + "s").c_str() : ONOFF(false));
  ESP_LOGCONFIG(TAG, "  Persistent connection: %s", ONOFF(this->persistent_connection_));
}

void TionBLEVPort::dump_config() { this->dump_component_config(TAG); }

void TionBLEVPort::on_ble_ready() {
  for (auto listener : this->listeners_) {
    auto api = static_cast<TionClimateComponentBase *>(listener);
    if (api->on_ready()) {
      api->on_poll();
    }
  }
  this->schedule_disconnect();
}

void TionBLEVPort::update() {
  if (this->is_connected()) {
    for (auto listener : this->listeners_) {
      auto api = static_cast<TionClimateComponentBase *>(listener);
      api->on_poll();
    }
    this->schedule_disconnect(this->state_timeout_);
  } else {
    this->connect();
  }
}

void TionBLEVPort::schedule_poll() {
  if (this->is_persistent_connection() && this->is_connected()) {
    for (auto listener : this->listeners_) {
      auto api = static_cast<TionClimateComponentBase *>(listener);
      api->on_poll();
    }
  } else {
    this->connect();
  }
}

void TionBLEVPort::frame_state_disconnect(uint16_t frame_type) {
  for (auto listener : this->listeners_) {
    auto api = static_cast<TionClimateComponentBase *>(listener);
    if (this->state_frame_type_ == frame_type && frame_type != 0) {
      this->cancel_disconnect();
      if (!this->is_persistent_connection()) {
        this->schedule_disconnect();
      }
      break;
    }
  }
}

void TionBLEVPort::schedule_disconnect(uint32_t timeout) {
  if (timeout) {
    this->set_timeout(TAG, timeout, [this]() {
      ESP_LOGV(TAG, "Disconnecting");
      this->disconnect();
    });
  }
}

void TionBLEVPort::cancel_disconnect() { this->cancel_timeout(TAG); }

}  // namespace tion
}  // namespace esphome
