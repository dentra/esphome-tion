#include "esphome/core/log.h"
#include "esphome/core/defines.h"
#ifdef USE_OTA
#include "esphome/components/ota/ota_component.h"
#endif

#include "tion_4s_uart.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_4s_uart";

void Tion4sUartVPort::dump_config() {
  VPORT_UART_LOG("Tion 4S UART");
  ESP_LOGCONFIG(TAG, "  Heartbeat Interval: %.1fs", this->heartbeat_interval_ / 1000.0f);
}

void Tion4sUartVPort::setup() {
  if (this->api_ == nullptr) {
    ESP_LOGE(TAG, "api is not configured");
    this->mark_failed();
    return;
  }

  this->set_interval(this->heartbeat_interval_, [this]() { this->api_->send_heartbeat(); });

#ifdef USE_OTA
  // additionally send heartbeat when OTA starts and before ESP restart.
  ota::global_ota_component->add_on_state_callback([this](ota::OTAState state, float, uint8_t) {
    if (state != ota::OTAState::OTA_IN_PROGRESS) {
      this->api_->send_heartbeat();
    }
  });
#endif
}

}  // namespace tion
}  // namespace esphome
