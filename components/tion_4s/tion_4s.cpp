#include "esphome/core/log.h"
#include "esphome/core/defines.h"
#ifdef USE_OTA
#include "esphome/components/ota/ota_component.h"
#endif

#include "tion_4s.h"

namespace esphome {
namespace tion {
static const char *const TAG = "tion_4s";

void Tion4sApiComponent::dump_config() {
  TionApiComponentBase<Tion4sApi>::dump_config();
  ESP_LOGCONFIG(TAG, "  Heartbeat Interval: %.1fs", this->heartbeat_interval_ / 1000.0f);
}

void Tion4sApiComponent::setup() {
  TionApiComponentBase<Tion4sApi>::setup();

#ifdef TION_ENABLE_HEARTBEAT
  if (this->vport_type_ == TionVPortType::VPORT_UART) {
    this->set_interval(this->heartbeat_interval_, [this]() { this->typed_api()->send_heartbeat(); });

#ifdef USE_OTA
    if (this->heartbeat_interval_ > 0) {
      // additionally send heartbeat when OTA starts and before ESP restart.
      ota::global_ota_component->add_on_state_callback([this](ota::OTAState state, float, uint8_t) {
        if (state != ota::OTAState::OTA_IN_PROGRESS) {
          this->typed_api()->send_heartbeat();
        }
      });
    }
#endif
  }
#endif  // TION_ENABLE_HEARTBEAT
}

}  // namespace tion
}  // namespace esphome
