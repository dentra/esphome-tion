#pragma once

#include "esphome/core/preferences.h"

#include "../tion_ble/tion_ble.h"
#include "../tion-api/tion-api-3s.h"
#include "../tion-api/tion-api-ble-3s.h"
#include "../tion-api/tion-api-ble.h"

namespace esphome {
namespace tion {

class Tion3sBLEVPortBase : public TionBLEVPortBase {
  using parent_type = TionBLEVPortBase;

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

  bool ble_reg_for_notify() const override {
    // при постоянном подключении требуется попреподключение иначе мы никогде не получим notify
    if (this->is_persistent_connection()) {
      return true;
    }
    return this->pair_state_ > 0;
  }

  bool write_data(const uint8_t *data, size_t size) { return parent_type::write_data(data, size); }
  bool read_frame(uint16_t type, const void *data, size_t size) { return parent_type::read_frame(type, data, size); }

 protected:
  ESPPreferenceObject rtc_;
  int8_t pair_state_{};  // 0: not paired, >0: paired, <0: pairing
  bool experimental_always_pair_{};
  dentra::tion::TionsApi3s *api_{};
};

class VPortTionBle3sProtocol final : public dentra::tion::TionBleProtocol<dentra::tion::TionBle3sProtocol> {
 public:
  esp_ble_sec_act_t get_ble_encryption() const { return esp_ble_sec_act_t::ESP_BLE_SEC_ENCRYPT; }
  bool write_frame(uint16_t type, const void *data, size_t size) {
    return dentra::tion::TionBle3sProtocol::write_frame(type, data, size);
  }
};

template<class protocol_type = VPortTionBle3sProtocol>
class Tion3sBLEVPort final : public TionBLEVPortT<VPortTionBle3sProtocol, Tion3sBLEVPortBase> {
  static_assert(std::is_same<VPortTionBle3sProtocol, protocol_type>::value,
                "protocol_type must be a VPortTionBle3sProtocol class");

 public:
  explicit Tion3sBLEVPort(VPortTionBle3sProtocol *protocol) : TionBLEVPortT(protocol) {}
};

}  // namespace tion
}  // namespace esphome
