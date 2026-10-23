#include <cinttypes>
#include "esphome/core/defines.h"
#include "esphome/core/log.h"
#include <esp_mac.h>
#include "esphome/components/esp32_ble_server/ble_2902.h"

// #include "../tion-api/tion-api-internal.h"
// #ifdef USE_TION_RC_3S
// #include "../tion-api/tion-api-3s-internal.h"
// #endif

#include "tion_rc.h"

#ifndef USE_TION_RC_EMU
#include "../tion/tion_component.h"
#endif

namespace esphome {
namespace tion_rc {

static const char *const TAG = "tion_rc";

TionRC::TionRC(tion::TionApiComponent *tion, TionRCControl *control) : control_(control) {
  this->control_->set_writer([this](const uint8_t *data, size_t size) {
    TION_RC_DUMP(TAG, "TX RC: %s", format_hex_pretty(data, size).c_str());
    if (this->char_notify_) {
      this->char_notify_->set_value(std::vector<uint8_t>(data, data + size));
      this->char_notify_->notify();
    }
    return true;
  });

#ifndef USE_TION_RC_EMU
  tion->add_on_state_callback([this](const dentra::tion::TionState *state) {
    if (state && this->control_->has_state_req()) {
      this->control_->on_state(*state);
    }
  });
#endif
}

void TionRC::setup() {
  global_ble_server->on_connect([this](uint16_t) { this->on_client_connect(); });
  global_ble_server->on_disconnect([this](uint16_t) { this->on_client_disconnect(); });
}

void TionRC::setup_service_() {
  ESP_LOGD(TAG, "Setting up BLE service...");

  const auto service_uuid = ESPBTUUID::from_raw(this->control_->get_ble_service());
  this->service_ = global_ble_server->create_service(service_uuid, true);

  // смена порядка создания сервиса ведет к тому, что паблишится корявый notify-дескриптор

  this->char_notify_ =
      this->service_->create_characteristic(this->control_->get_ble_char_rx(), BLECharacteristic::PROPERTY_NOTIFY);
  this->char_notify_->add_descriptor(new BLE2902());  // add notify descriptor

  auto *char_rx =
      this->service_->create_characteristic(this->control_->get_ble_char_tx(), BLECharacteristic::PROPERTY_WRITE);
  // не очень понятно почему не применяется в конструкторе, поэтому попробуем так
  char_rx->set_write_no_response_property(true);
  char_rx->on_write([this](std::span<const uint8_t> data, uint16_t) {
    if (!data.empty()) {
      TION_RC_DUMP(TAG, "RX RC: %s", format_hex_pretty(data.data(), data.size()).c_str());
      this->control_->pr_read_data(data.data(), data.size());
    }
  });

  this->state_ = State::INITIALIZED;
}

void TionRC::adv(bool pair) {
  // esphome 2026 advertises only with a public address and does not advertise services created at runtime,
  // so the advertising is fully controlled here. esphome does not touch it while nobody requested
  // global_ble->advertising_start(); it may stop it on connect/disconnect, we restart it on disconnect.
  esp_ble_gap_stop_advertising();

  esp_bd_addr_t addr;
  bool random_addr = this->control_->use_random_address();
  if (this->has_address_) {
    memcpy(addr, this->address_, sizeof(addr));
    // configured random static address (two most significant bits are 1), public otherwise
    random_addr = (addr[0] & 0xC0) == 0xC0;
  } else {
    esp_read_mac(addr, ESP_MAC_BT);
    if (random_addr) {
      // random static address: two most significant bits are 1
      addr[0] |= 0xC0;
    }
  }
  if (random_addr) {
    esp_ble_gap_set_rand_addr(addr);
  } else if (this->has_address_) {
    ESP_LOGW(TAG, "Configured address is not random static, advertising with the public one");
    esp_read_mac(addr, ESP_MAC_BT);
  }
  this->control_->set_address(addr);
  this->control_->adv(pair);

  // Для 4S scan response содержит следующую информацию:
  // Bluetooth HCI Event - LE Meta
  //     Event Code: LE Meta (0x3e)
  //     Parameter Total Length: 30
  //     Sub Event: LE Advertising Report (0x02)
  //     Num Reports: 1
  //     Event Type: Scan Response (0x04)
  //     Peer Address Type: Random Device Address (0x01)
  //     BD_ADDR: xx:xx:xx:xx:xx:xx (xx:xx:xx:xx:xx:xx)
  //     Data Length: 18
  //     Advertising Data
  //         128-bit Service Class UUIDs
  //             Length: 17
  //             Type: 128-bit Service Class UUIDs (0x07)
  //             Custom UUID: 98f00001-3788-83ea-453e-f52244709ddb (Unknown)
  //     RSSI: -56 dBm

  const auto uuid128bit = this->service_->get_uuid().as_128bit();
  esp_bt_uuid_t uuid = uuid128bit.get_uuid();

  esp_ble_adv_data_t adv_scan_rsp_data{};
  adv_scan_rsp_data.set_scan_rsp = true;
  adv_scan_rsp_data.service_uuid_len = uuid.len;
  adv_scan_rsp_data.p_service_uuid = uuid.uuid.uuid128;
  esp_ble_gap_config_adv_data(&adv_scan_rsp_data);

  esp_ble_adv_params_t adv_params{
      .adv_int_min = 0x20,
      .adv_int_max = 0x40,
      .adv_type = ADV_TYPE_IND,
      .own_addr_type = random_addr ? BLE_ADDR_TYPE_RANDOM : BLE_ADDR_TYPE_PUBLIC,
      .peer_addr = {},
      .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
      .channel_map = ADV_CHNL_ALL,
      .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
  };
  const esp_err_t err = esp_ble_gap_start_advertising(&adv_params);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "esp_ble_gap_start_advertising failed: %s", esp_err_to_name(err));
  }

  ESP_LOGD(TAG, "Advertising started. pair=%s, address=%02X:%02X:%02X:%02X:%02X:%02X (%s)", ONOFF(pair), addr[0],
           addr[1], addr[2], addr[3], addr[4], addr[5], random_addr ? "random" : "public");
}

void TionRC::loop() {
  this->control_->loop();

  if (!global_ble_server->is_running()) {
    this->state_ = State::STOPPED;
    return;
  }

  if (!this->service_) {
    this->setup_service_();
    return;
  }

  switch (this->state_) {
    case State::STOPPED:
      break;

    case State::INITIALIZED:
      if (this->service_->is_created()) {
        ESP_LOGD(TAG, "Starting up BLE service...");
        this->service_->start();
        if (this->service_->is_starting()) {
          this->state_ = State::STARTING;
        }
      }
      break;

    case State::STARTING:
      if (this->service_->is_running()) {
        this->adv(this->pair_mode_ ? this->pair_mode_->state : false);
        this->state_ = State::STARTED;
      }
      break;

    case State::STARTED: {
      break;
    }
  }
}

void TionRC::stop() {
  if (this->service_) {
    this->service_->stop();
  }
  this->state_ = State::STOPPED;
}

void TionRC::on_client_connect() {
  ESP_LOGD(TAG, "Client connected");
  // advertising is stopped by the stack on connect, it is restarted after disconnect (State::STARTING)
  if (this->pair_mode_) {
    this->pair_mode_->publish_state(false);
  }
}

void TionRC::on_client_disconnect() {
  ESP_LOGD(TAG, "Client disconnected");
  this->state_ = State::STARTING;
}

void TionRC::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
  switch (event) {
    case ESP_GAP_BLE_SEC_REQ_EVT: {
      /* send the positive (true) security response to the peer device to accept the security request.
      If not accept the security request, should send the security response with negative(false) accept value*/
      esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
      break;
    }

    default:
      break;
  }
}

void TionRC::gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
  switch (event) {
    case ESP_GATTS_CONNECT_EVT: {
      // start security connect with peer device when receive the connect event sent by the master.
      esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
      break;
    }

    case ESP_GATTS_REG_EVT: {
      esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
      esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
      uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_ENABLE;
      esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(auth_option));
      uint8_t key_size = 16;
      esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(key_size));
      uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
      esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(init_key));
      uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
      esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(rsp_key));
      break;
    }

    default:
      break;
  }
}

}  // namespace tion_rc
}  // namespace esphome
