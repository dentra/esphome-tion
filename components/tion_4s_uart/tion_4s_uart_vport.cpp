#include "esphome/core/log.h"
#include "esphome/core/defines.h"
#ifdef USE_OTA
#if (ESPHOME_VERSION_CODE < VERSION_CODE(2024, 6, 0))
#include "esphome/components/ota/ota_component.h"
#else
#include "esphome/components/ota/ota_backend.h"
#endif
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
#if (ESPHOME_VERSION_CODE < VERSION_CODE(2024, 6, 0))
  auto *global_ota_callback = ota::global_ota_component;
#else
  auto *global_ota_callback = ota::get_global_ota_callback();
#endif
  // additionally send heartbeat when OTA starts and before ESP restart.
  global_ota_callback->add_on_state_callback([this](ota::OTAState state, float, uint8_t) {
    if (state != ota::OTAState::OTA_IN_PROGRESS) {
      this->api_->send_heartbeat();
    }
  });
#endif
}

}  // namespace tion
}  // namespace esphome
