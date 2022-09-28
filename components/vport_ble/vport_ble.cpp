#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "vport_ble.h"

namespace esphome {
namespace vport {

static const char *const TAG = "vport_ble";

void VPortBLENode::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                       esp_ble_gattc_cb_param_t *param) {
  if (event == ESP_GATTC_NOTIFY_EVT) {
    ESP_LOGV(TAG, "Got notify for handle %04u: %s", param->notify.handle,
             format_hex_pretty(param->notify.value, param->notify.value_len).c_str());
    if (param->notify.handle == this->char_rx_) {
      this->on_ble_data(param->notify.value, param->notify.value_len);
    }
    return;
  }

  if (event == ESP_GATTC_WRITE_DESCR_EVT) {
    ESP_LOGV(TAG, "write_char_descr at 0x%x complete 0x%02x", param->write.handle, param->write.status);
    this->node_state = esp32_ble_tracker::ClientState::ESTABLISHED;
    this->on_ble_ready_();
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
      this->on_ble_ready_();
    }

    return;
  }

  if (event == ESP_GATTC_CONNECT_EVT) {
    // let the device to do pair
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

void VPortBLENode::on_ble_ready_() {
  this->write_queue();
  this->on_ble_ready();
}

void VPortBLENode::write_queue() {
#ifdef VPORT_BLE_ENABLE_QUEUE
  for (const auto &el : this->waited_) {
    this->write_ble_data_(el.data(), el.size());
  }
  this->waited_.clear();
#endif
}

bool VPortBLENode::write_ble_data(const uint8_t *data, uint16_t size) {
  if (this->is_connected()) {
    this->write_queue();
    return this->write_ble_data_(data, size);
  }
#ifdef VPORT_BLE_ENABLE_QUEUE

  this->waited_.push_back(std::vector<uint8_t>(data, data + size));
  if (this->waited_.size() > this->get_max_queue_size()) {
    this->waited_.erase(this->waited_.begin());
  }
  this->connect();
  return true;
#else
  ESP_LOGW(TAG, "Not connected, can't write: %s", format_hex_pretty(data, size).c_str());
  return false;
#endif
}

bool VPortBLENode::write_ble_data_(const uint8_t *data, uint16_t size) const {
  ESP_LOGV(TAG, "Writing data to 0x%x: %s", this->char_tx_, format_hex_pretty(data, size).c_str());
#ifdef ESPHOME_LOG_HAS_VERBOSE
  auto write_type = ESP_GATT_WRITE_TYPE_RSP;
#else
  auto write_type = ESP_GATT_WRITE_TYPE_NO_RSP;
#endif
  return esp_ble_gattc_write_char(this->parent_->gattc_if, this->parent_->conn_id, this->char_tx_, size,
                                  const_cast<uint8_t *>(data), write_type, ESP_GATT_AUTH_REQ_NONE) == ESP_OK;
}

void VPortBLENode::schedule_disconnect(Component *component, uint32_t timeout) {
  if (timeout) {
    App.scheduler.set_timeout(component, TAG, timeout, [this]() {
      ESP_LOGV(TAG, "Disconnecting");
      this->disconnect();
    });
  }
}

void VPortBLENode::cancel_disconnect(Component *component) { App.scheduler.cancel_timeout(component, TAG); }

}  // namespace vport
}  // namespace esphome
