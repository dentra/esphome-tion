#include "esphome/core/log.h"

#include "../tion/tion.h"
#include "tion_uart.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_uart";

void TionUARTVPort::dump_config() {
  ESP_LOGCONFIG(TAG, "Virtual UART port:");
  ESP_LOGCONFIG(TAG, "  Update interval: %u s", this->update_interval_ / 1000);
  ESP_LOGCONFIG(
      TAG, "  Heartbeat interval: %s",
      this->heartbeat_interval_ > 0 ? (to_string(this->heartbeat_interval_ / 1000) + " s").c_str() : ONOFF(false));
}

void TionUARTVPort::setup() {
  for (auto listener : this->listeners_) {
    auto api = static_cast<TionClimateComponentBase *>(listener);
    api->on_ready();
  }
  if (this->heartbeat_interval_ > 0) {
    this->set_interval(this->heartbeat_interval_, [this]() {
      for (auto listener : this->listeners_) {
        auto api = static_cast<TionClimateComponentBase *>(listener);
        api->send_heartbeat();
        break;
      }
    });
  }
}

void TionUARTVPort::update() {
  ESP_LOGV(TAG, "Run polling");
  for (auto listener : this->listeners_) {
    auto api = static_cast<TionClimateComponentBase *>(listener);
    api->on_poll();
  }
}

bool VPortTionUartProtocol::read_frame(uint16_t type, const void *data, size_t size) {
  ESP_LOGV(TAG, "Read frame 0x%04X: %s", type, format_hex_pretty(static_cast<const uint8_t *>(data), size).c_str());
  parent_->fire_listeners(type, data, size);
  return true;
}

bool VPortTionUartProtocol::write_data(const uint8_t *data, size_t size) const {
  ESP_LOGV(TAG, "Write data: %s", format_hex_pretty(data, size).c_str());
  parent_->uart_->write_array(data, size);
  parent_->uart_->flush();
  return true;
}

}  // namespace tion
}  // namespace esphome
