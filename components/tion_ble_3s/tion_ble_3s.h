#pragma once

#include "esphome/core/preferences.h"

#include "../tion_ble/tion_ble.h"
#include "../tion-api/tion-api-3s.h"

namespace esphome {
namespace tion {

class Tion3sBLEVPort final : public TionBLEVPort {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;

  void pair();
  void reset_pair();

  void set_api(dentra::tion::TionsApi3s *api) { this->api_ = api; }
  void set_experimental_always_pair(bool value) { this->experimental_always_pair_ = value; }

  // VPortBLEComponent implementation
  void on_ble_ready() override;

  bool ble_reg_for_notify() const override { return this->pair_state_ > 0; }

 protected:
  ESPPreferenceObject rtc_;
  int8_t pair_state_{};  // 0: not paired, >0: paired, <0: pairing
  bool experimental_always_pair_{};
  dentra::tion::TionsApi3s *api_;
};

class VPortTionBle3sProtocol final : public VPortTionBleProtocol<dentra::tion::TionBle3sProtocol> {
 public:
  VPortTionBle3sProtocol(TionBLEVPort *vport) : VPortTionBleProtocol(vport) {}
  esp_ble_sec_act_t get_ble_encryption() override { return esp_ble_sec_act_t::ESP_BLE_SEC_ENCRYPT; }
};

}  // namespace tion
}  // namespace esphome
