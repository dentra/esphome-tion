#include "esphome/core/log.h"
#include "vport_ble.h"

namespace esphome {
namespace vport {

static const char *const TAG = "vport_ble";

void VPortBLENode::dump_setting(const char *TAG) {
  ESP_LOGCONFIG(TAG, "  BLE Service: %s", this->ble_service_.to_string().c_str());
  ESP_LOGCONFIG(TAG, "  BLE Char TX: %s", this->ble_char_tx_.to_string().c_str());
  ESP_LOGCONFIG(TAG, "  BLE Char RX: %s", this->ble_char_rx_.to_string().c_str());
  ESP_LOGCONFIG(TAG, "  BLE Sec Act: %u", this->ble_sec_act_);
}

void VPortBLENode::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                       esp_ble_gattc_cb_param_t *param) {
  if (event == ESP_GATTC_NOTIFY_EVT) {
    ESP_LOGV(TAG, "Got notify for handle %04u: %s", param->notify.handle,
             format_hex_pretty(param->notify.value, param->notify.value_len).c_str());
    if (param->notify.handle == this->char_rx_) {
      this->read_ble_data(param->notify.value, param->notify.value_len);
    }
    return;
  }

  if (event == ESP_GATTC_WRITE_DESCR_EVT) {
    ESP_LOGV(TAG, "write_char_descr at 0x%x complete 0x%02x", param->write.handle, param->write.status);
    this->node_state = esp32_ble_tracker::ClientState::ESTABLISHED;
    this->on_ble_ready();
    return;
  }

  if (event == ESP_GATTC_REG_FOR_NOTIFY_EVT) {
    ESP_LOGV(TAG, "Registring for notify complete");
    return;
  }

  if (event == ESP_GATTC_SEARCH_CMPL_EVT) {
    auto tx = this->parent()->get_characteristic(this->ble_service_, this->ble_char_tx_);
    if (tx == nullptr) {
      ESP_LOGE(TAG, "Can't discover TX characteristics");
      return;
    }
    this->char_tx_ = tx->handle;

    auto rx = this->parent()->get_characteristic(this->ble_service_, this->ble_char_rx_);
    if (rx == nullptr) {
      ESP_LOGE(TAG, "Can't discover RX characteristics");
      return;
    }
    this->char_rx_ = rx->handle;

    ESP_LOGD(TAG, "Discovering complete");
    ESP_LOGV(TAG, "  TX handle 0x%x", this->char_tx_);
    ESP_LOGV(TAG, "  RX handle 0x%x", this->char_rx_);

    if (this->ble_reg_for_notify()) {
      auto err = esp_ble_gattc_register_for_notify(this->parent_->gattc_if, this->parent_->remote_bda, this->char_rx_);
      ESP_LOGV(TAG, "Register for notify 0x%x complete: %s", this->char_rx_, YESNO(err == ESP_OK));
    } else {
      this->node_state = esp32_ble_tracker::ClientState::ESTABLISHED;
      this->on_ble_ready();
    }

    return;
  }

  if (event == ESP_GATTC_CONNECT_EVT) {
    // let device to pair
    if (this->ble_sec_act_ != 0) {
      auto err = esp_ble_set_encryption(param->connect.remote_bda, this->ble_sec_act_);
      ESP_LOGV(TAG, "Bonding complete: %s", YESNO(err == ESP_OK));
    } else {
      ESP_LOGV(TAG, "Bonding skipped");
    }
    return;
  }

#ifdef ESPHOME_LOG_HAS_VERBOSE
  if (event == ESP_GATTC_WRITE_CHAR_EVT) {
    ESP_LOGV(TAG, "write_char at 0x%x complete 0x%02x", param->write.handle, param->write.status);
    return;
  }

  if (event == ESP_GATTC_ENC_CMPL_CB_EVT) {
    ESP_LOGV(TAG, "esp_ble_set_encryption complete");
    return;
  }
#endif
}

bool VPortBLENode::write_ble_data(const uint8_t *data, uint16_t size) const {
  ESP_LOGV(TAG, "write_data to 0x%x: %s", this->char_tx_, format_hex_pretty(data, size).c_str());
#ifdef ESPHOME_LOG_HAS_VERBOSE
  esp_gatt_write_type_t write_type = ESP_GATT_WRITE_TYPE_RSP;
#else
  esp_gatt_write_type_t write_type = ESP_GATT_WRITE_TYPE_NO_RSP;
#endif
  return esp_ble_gattc_write_char(this->parent_->gattc_if, this->parent_->conn_id, this->char_tx_, size,
                                  const_cast<uint8_t *>(data), write_type, ESP_GATT_AUTH_REQ_NONE) == ESP_OK;
}

}  // namespace vport
}  // namespace esphome
