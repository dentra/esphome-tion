#include "esphome/core/log.h"
#include "esphome/core/defines.h"
#ifdef USE_OTA
#include "esphome/components/ota/ota_component.h"
#endif

#include "tion_uart.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_uart";

void TionUARTVPort::dump_config() {
  VPORT_UART_LOG(TAG, "Tion UART");
  ESP_LOGCONFIG(
      TAG, "  Heartbeat interval: %s",
      this->heartbeat_interval_ > 0 ? (to_string(this->heartbeat_interval_ / 1000) + " s").c_str() : ONOFF(false));
}

void TionUARTVPort::setup() {
  this->fire_ready();
  if (this->heartbeat_interval_ > 0 && this->cc_ != nullptr) {
    this->set_interval(this->heartbeat_interval_, [this]() { this->cc_->send_heartbeat(); });
#ifdef USE_OTA
    ota::global_ota_component->add_on_state_callback(
        [this](ota::OTAState, float, uint8_t) { this->cc_->send_heartbeat(); });
#endif
  }
}

void TionUARTVPort::update() { this->fire_poll(); }

bool TionUARTVPort::read_frame(uint16_t type, const void *data, size_t size) {
  ESP_LOGV(TAG, "Read frame 0x%04X: %s", type, format_hex_pretty(static_cast<const uint8_t *>(data), size).c_str());
  this->fire_frame(type, data, size);
  return true;
}

bool TionUARTVPort::write_data(const uint8_t *data, size_t size) {
  ESP_LOGV(TAG, "Write data: %s", format_hex_pretty(data, size).c_str());
  this->uart_->write_array(data, size);
  this->uart_->flush();
  return true;
}

}  // namespace tion
}  // namespace esphome
