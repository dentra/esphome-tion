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

#ifdef USE_OTA
  auto *global_ota_callback = ota::get_global_ota_callback();

  // дополнительно пинганем бризер при OTA обновлении
  global_ota_callback->add_on_state_callback([this](ota::OTAState state, float, uint8_t, ota::OTAComponent *) {
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
  });
#endif
}

void Tion4sUartVPort::on_shutdown() {
  // дополнительно пинганем бризер перед перезагрузкой
  this->api_->send_heartbeat();
  delay(20);  // дадим немного времени чтобы принять ответ
}

}  // namespace tion
}  // namespace esphome
