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
  TionUARTVPortBase::setup();

  if (this->heartbeat_interval_ > 0 && this->cc_ != nullptr) {
    this->set_interval(this->heartbeat_interval_, [this]() { this->cc_->send_heartbeat(); });
#ifdef USE_OTA
    // additionally send heartbean when OTA starts and before ESP restart.
    ota::global_ota_component->add_on_state_callback([this](ota::OTAState state, float, uint8_t) {
      if (state != ota::OTAState::OTA_IN_PROGRESS) {
        this->cc_->send_heartbeat();
      }
    });
#endif
  }
}

}  // namespace tion
}  // namespace esphome
