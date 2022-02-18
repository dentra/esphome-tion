#pragma once

#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

namespace esphome {
namespace ble_secure {

class BLESecureTracker : public esp32_ble_tracker::ESP32BLETracker {
 public:
  static void gap_event_handler_(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
  static void gattc_event_handler_(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
  static void secure_ble_setup_();

  std::vector<esp32_ble_tracker::ESPBTClient *> &get_clients() { return this->clients_; }
};

class BLESecure : public Component {
 protected:
 public:
  float get_setup_priority() const override;
  void setup() override;
  void loop() override;
};

class BLESecureClient : public esp32_ble_tracker::ESPBTClient {
 public:
  BLESecureClient(esp32_ble_tracker::ESP32BLETracker *tracker, esp32_ble_tracker::ESPBTClient *parent_client)
      : parent_client_(parent_client) {
    this->set_parent(tracker);
  };

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override {
    return this->parent_client_->parse_device(device);
  }
  void connect() override { this->parent_client_->connect(); };

  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t esp_gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;

 protected:
  esp32_ble_tracker::ESPBTClient *parent_client_;
};

}  // namespace ble_secure
}  // namespace esphome
