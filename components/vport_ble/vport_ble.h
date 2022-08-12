#pragma once

#include "esphome/components/ble_client/ble_client.h"
#include "../vport/vport.h"

namespace esphome {
namespace vport {

class VPortBLENode : public ble_client::BLEClientNode {
 public:
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;

  void dump_setting(const char *TAG);

  virtual void on_ble_ready() = 0;
  virtual void read_ble_data(const uint8_t *data, uint16_t size) = 0;
  virtual bool write_ble_data(const uint8_t *data, uint16_t size) const;

  void set_ble_service(const char *ble_service) {
    this->ble_service_ = esp32_ble_tracker::ESPBTUUID::from_raw(ble_service);
  }
  void set_ble_char_tx(const char *ble_char_tx) {
    this->ble_char_tx_ = esp32_ble_tracker::ESPBTUUID::from_raw(ble_char_tx);
  }
  void set_ble_char_rx(const char *ble_char_rx) {
    this->ble_char_rx_ = esp32_ble_tracker::ESPBTUUID::from_raw(ble_char_rx);
  }
  void set_ble_encryption(esp_ble_sec_act_t ble_sec_act) { this->ble_sec_act_ = ble_sec_act; }

  virtual bool ble_reg_for_notify() const { return true; }

  bool is_connected() const { return this->node_state == esp32_ble_tracker::ClientState::ESTABLISHED; }
  void connect() { this->parent()->set_enabled(true); }
  void disconnect() { this->parent()->set_enabled(false); }

 protected:
  uint16_t char_rx_{};
  uint16_t char_tx_{};
  esp32_ble_tracker::ESPBTUUID ble_service_{};
  esp32_ble_tracker::ESPBTUUID ble_char_tx_{};
  esp32_ble_tracker::ESPBTUUID ble_char_rx_{};
  esp_ble_sec_act_t ble_sec_act_{esp_ble_sec_act_t::ESP_BLE_SEC_ENCRYPT_MITM};
};

template<typename T> class VPortBLEComponent : public VPortComponent<T>, public VPortBLENode {
  //  public:
  //   void set_state_timeout(uint32_t state_timeout) { this->state_timeout_ = state_timeout; }
  //   void set_persistent_connection(bool persistent_connection) { this->persistent_connection_ =
  //   persistent_connection; } bool is_persistent_connection() const { return this->persistent_connection_; }

  //  protected:
  //   uint32_t state_timeout_{};
  //   bool persistent_connection_{};
};

}  // namespace vport
}  // namespace esphome
