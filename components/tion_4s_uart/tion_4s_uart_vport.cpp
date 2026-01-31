#include "esphome/core/log.h"
#include "esphome/core/defines.h"

#ifdef USE_OTA
#include "esphome/components/ota/ota_backend.h"
#endif

#include "tion_4s_uart_vport.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_4s_uart_vport";

void Tion4sUartVPort::dump_config() {
  VPORT_UART_LOG("Tion 4S UART");
  ESP_LOGCONFIG(TAG, "  Heartbeat Interval: %.1f s", this->heartbeat_interval_ * 0.001f);
}

void Tion4sUartVPort::setup() {
  if (this->api_ == nullptr) {
    ESP_LOGE(TAG, "api is not configured");
    this->mark_failed();
    return;
  }

  this->set_interval(this->heartbeat_interval_, [this]() { this->api_->send_heartbeat(); });
}

void Tion4sUartVPort::on_shutdown() {
  // дополнительно пинганем бризер перед перезагрузкой
  this->api_->send_heartbeat();
  delay(20);  // дадим немного времени чтобы принять ответ
}

#ifdef USE_OTA_STATE_LISTENER
void Tion4sUartVPort::on_ota_global_state(ota::OTAState state, float progress, uint8_t error, ota::OTAComponent *comp) {
  // дополнительно пинганем бризер при OTA обновлении
  static uint32_t tm{};
  if (state == ota::OTAState::OTA_STARTED) {
    // при старте
    tm = millis();
    this->api_->send_heartbeat();
  } else {
    uint32_t ct = millis();
    if (ct - tm > this->heartbeat_interval_) {
      // раз в heartbeat_interval
      this->api_->send_heartbeat();
      tm = ct;
    }
  }
}
#endif

}  // namespace tion
}  // namespace esphome
