#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#include "tion_3s.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_3s";

// 6e400001-b5a3-f393-e0a9-e50e24dcca9e
static const esp_bt_uuid_t BLE_TION3S_SERVICE{
    .len = ESP_UUID_LEN_128,
    .uuid = {
        .uuid128 = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E},
    }};

// 6e400002-b5a3-f393-e0a9-e50e24dcca9e
static const esp_bt_uuid_t BLE_TION3S_CHAR_TX{
    .len = ESP_UUID_LEN_128,
    .uuid = {
        .uuid128 = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E},
    }};

// 6e400003-b5a3-f393-e0a9-e50e24dcca9e
static const esp_bt_uuid_t BLE_TION3S_CHAR_RX{
    .len = ESP_UUID_LEN_128,
    .uuid = {
        .uuid128 = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E},
    }};

const esp_bt_uuid_t &Tion3s::get_ble_service() const { return BLE_TION3S_SERVICE; }
const esp_bt_uuid_t &Tion3s::get_ble_char_tx() const { return BLE_TION3S_CHAR_TX; }
const esp_bt_uuid_t &Tion3s::get_ble_char_rx() const { return BLE_TION3S_CHAR_RX; }

class SecureESP32BLETracker : public esp32_ble_tracker::ESP32BLETracker {
 protected:
  static const char *esp_auth_req_to_str(esp_ble_auth_req_t auth_req) {
    const char *auth_str = NULL;
    switch (auth_req) {
      case ESP_LE_AUTH_NO_BOND:
        auth_str = "ESP_LE_AUTH_NO_BOND";
        break;
      case ESP_LE_AUTH_BOND:
        auth_str = "ESP_LE_AUTH_BOND";
        break;
      case ESP_LE_AUTH_REQ_MITM:
        auth_str = "ESP_LE_AUTH_REQ_MITM";
        break;
      case ESP_LE_AUTH_REQ_SC_ONLY:
        auth_str = "ESP_LE_AUTH_REQ_SC_ONLY";
        break;
      case ESP_LE_AUTH_REQ_SC_BOND:
        auth_str = "ESP_LE_AUTH_REQ_SC_BOND";
        break;
      case ESP_LE_AUTH_REQ_SC_MITM:
        auth_str = "ESP_LE_AUTH_REQ_SC_MITM";
        break;
      case ESP_LE_AUTH_REQ_SC_MITM_BOND:
        auth_str = "ESP_LE_AUTH_REQ_SC_MITM_BOND";
        break;
      default:
        auth_str = "INVALID BLE AUTH REQ";
        break;
    }

    return auth_str;
  }
  static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    esp32_ble_tracker::ESP32BLETracker::gap_event_handler(event, param);
    if (event == ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT) {
      if (param->local_privacy_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "config local privacy failed, error code =%x", param->local_privacy_cmpl.status);
        //} else {
        // esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        // if (scan_ret) {
        //   ESP_LOGE(TAG, "set scan params error, error code = %x", scan_ret);
        // }
      }
    } else if (event == ESP_GAP_BLE_SEC_REQ_EVT) {
      ESP_LOGD(TAG, "GAP BLE security request");
      /* send the positive(true) security response to the peer device to accept the security request.
        If not accept the security request, should sent the security response with negative(false) accept value*/
      esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
      // } else if (event == ESP_GAP_BLE_NC_REQ_EVT) {
      /* The app will receive this evt when the IO has DisplayYesNO capability and the peer device IO also has
        DisplayYesNo capability. show the passkey number to the user to confirm it with the number displayed by peer
        deivce. */
      // ESP_LOGI(TAG, "ESP_GAP_BLE_NC_REQ_EVT, the passkey Notify number: %d", param->ble_security.key_notif.passkey);
      // } else if (event == ESP_GAP_BLE_PASSKEY_NOTIF_EVT) {
      /// the app will receive this evt when the IO has Output capability and the peer device IO has Input capability.
      /// show the passkey number to the user to input it in the peer deivce.
      // ESP_LOGI(TAG, "The passkey Notify number: %06d", param->ble_security.key_notif.passkey);
      // } else if (event == ESP_GAP_BLE_KEY_EVT) {
      // shows the ble key info share with peer device to the user.
      // ESP_LOGV(TAG, "key type = %s", esp_key_type_to_str(param->ble_security.ble_key.key_type));
    } else if (event == ESP_GAP_BLE_AUTH_CMPL_EVT) {
      ESP_LOGD(TAG, "GAP BLE auth complete");
      const esp_bd_addr_t &bd_addr = param->ble_security.auth_cmpl.bd_addr;
      ESP_LOGD("  ", "remote BD_ADDR: %08x%04x",
               (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
               (bd_addr[4] << 8) + bd_addr[5]);
      ESP_LOGD("  ", "address type = %d", param->ble_security.auth_cmpl.addr_type);
      ESP_LOGD("  ", "pair status = %s", param->ble_security.auth_cmpl.success ? "success" : "fail");
      if (param->ble_security.auth_cmpl.success) {
        ESP_LOGD(TAG, "auth mode = %s", esp_auth_req_to_str(param->ble_security.auth_cmpl.auth_mode));
      } else {
        ESP_LOGD(TAG, "fail reason = 0x%x", param->ble_security.auth_cmpl.fail_reason);
      }
    }
  }

 public:
  static void secure_ble_setup() {
    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/

    esp_err_t err;

    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;  // bonding with peer device after authentication
    err = esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    ESP_LOGV(TAG, "Setting BLE_SM_AUTHEN_REQ_MODE: %s", YESNO(err == ESP_OK));

    // io cap mode already set during ESP32BLETracker::ble_setup()
    // esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;  // set the IO capability to No output No input
    // esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));

    uint8_t key_size = 16;  // the key size should be 7~16 bytes
    err = esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    ESP_LOGV(TAG, "Setting ESP_BLE_SM_MAX_KEY_SIZE: %s", YESNO(err == ESP_OK));

    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    err = esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    ESP_LOGV(TAG, "Setting ESP_BLE_SM_SET_INIT_KEY: %s", YESNO(err == ESP_OK));

    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    err = esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
    ESP_LOGV(TAG, "Setting ESP_BLE_SM_SET_RSP_KEY: %s", YESNO(err == ESP_OK));

    err = esp_ble_gap_register_callback(SecureESP32BLETracker::gap_event_handler);
    ESP_LOGV(TAG, "Registering esp_ble_gap_register_callback: %s", YESNO(err == ESP_OK));
  }
};

void Tion3s::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  TionBleNode::gattc_event_handler(event, gattc_if, param);
  if (event == ESP_GATTC_REG_EVT) {
    auto res = esp_ble_gap_config_local_privacy(true);
    ESP_LOGV("Enabling local privacy, result %s", YESNO(res == ESP_OK));
    return;
  }
}

void Tion3s::setup() {
  this->parent_->set_enabled(false);

  SecureESP32BLETracker::secure_ble_setup();

  this->rtc_ = global_preferences->make_preference<int8_t>(fnv1_hash(TAG), true);
  int8_t loaded{};
  if (this->rtc_.load(&loaded)) {
    this->pair_state_ = loaded;
  }
}

void Tion3s::on_ready() {
  if (this->pair_state_ == 0) {
    ESP_LOGD(TAG, "Not paired yet");
    this->parent_->set_enabled(false);
    return;
  }

  if (this->pair_state_ > 0) {
    bool res = this->request_state();
    ESP_LOGV(TAG, "Request state result: %s", YESNO(res));
    return;
  }

  bool res = TionsApi3s::pair();
  if (res) {
    this->pair_state_ = 1;
    this->rtc_.save(&this->pair_state_);
  }
  ESP_LOGD(TAG, "Pairing complete: %s", YESNO(res));
  App.scheduler.set_timeout(this, TAG, 3000, [this]() { this->parent_->set_enabled(false); });
}

void Tion3s::read(const tion3s_state_t &state) {
  if (this->dirty_) {
    this->dirty_ = false;
    tion3s_state_t st = state;
    this->update_state_(st);
    TionsApi3s::write_state(st);
    return;
  }

  if (state.flags.power_state) {
    this->mode = state.flags.heater_state ? climate::CLIMATE_MODE_HEAT : climate::CLIMATE_MODE_FAN_ONLY;
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
  }
  this->target_temperature = state.target_temperature;
  this->set_fan_mode_(state.fan_speed);
  this->publish_state();

  if (this->version_) {
    this->version_->publish_state(str_snprintf("%04X", 4, state.firmware_version));
  }
  if (this->buzzer_) {
    this->buzzer_->publish_state(state.flags.sound_state);
  }
  if (this->temp_in_) {
    this->temp_in_->publish_state(state.indoor_temperature);
  }
  if (this->temp_out_) {
    this->temp_out_->publish_state(state.outdoor_temperature2);
  }
  if (this->filter_days_left_) {
    this->filter_days_left_->publish_state(state.filter_days);
  }

  ESP_LOGV(TAG, "fan_speed    : %u", state.fan_speed);
  ESP_LOGV(TAG, "gate_position: %u", state.gate_position);
  ESP_LOGV(TAG, "target_temp  : %u", state.target_temperature);
  ESP_LOGV(TAG, "heater_state : %s", ONOFF(state.flags.heater_state));
  ESP_LOGV(TAG, "power_state  : %s", ONOFF(state.flags.power_state));
  ESP_LOGV(TAG, "timer_state  : %s", ONOFF(state.flags.timer_state));
  ESP_LOGV(TAG, "sound_state  : %s", ONOFF(state.flags.sound_state));
  ESP_LOGV(TAG, "auto_state   : %s", ONOFF(state.flags.auto_state));
  ESP_LOGV(TAG, "ma_connect   : %s", ONOFF(state.flags.ma_connect));
  ESP_LOGV(TAG, "save         : %s", ONOFF(state.flags.save));
  ESP_LOGV(TAG, "ma_pairing   : %s", ONOFF(state.flags.ma_pairing));
  ESP_LOGV(TAG, "reserved     : 0x%02X", state.flags.reserved);
  ESP_LOGV(TAG, "outdoor_temp1: %d", state.outdoor_temperature1);
  ESP_LOGV(TAG, "outdoor_temp2: %d", state.outdoor_temperature2);
  ESP_LOGV(TAG, "indoor_temp  : %d", state.indoor_temperature);
  ESP_LOGV(TAG, "filter_time  : %u", state.filter_time);
  ESP_LOGV(TAG, "hours        : %u", state.hours);
  ESP_LOGV(TAG, "minutes      : %u", state.minutes);
  ESP_LOGV(TAG, "last_error   : %u", state.last_error);
  ESP_LOGV(TAG, "productivity : %u", state.productivity);
  ESP_LOGV(TAG, "filter_days  : %u", state.filter_days);
  ESP_LOGV(TAG, "firmware     : %04X", state.firmware_version);

  // leave 3 sec connection left for end all of jobs
  App.scheduler.set_timeout(this, TAG, 3000, [this]() { this->parent_->set_enabled(false); });
}

void Tion3s::update_state_(tion3s_state_t &state) {
  if (this->custom_fan_mode.has_value()) {
    state.fan_speed = this->get_fan_speed();
  }

  state.target_temperature = this->target_temperature;
  state.flags.power_state = this->mode != climate::CLIMATE_MODE_OFF;
  state.flags.heater_state = this->mode == climate::CLIMATE_MODE_HEAT;

  if (this->buzzer_) {
    state.flags.sound_state = this->buzzer_->state;
  }
}

bool Tion3s::write_state() {
  if (this->direct_write_) {
    tion3s_state_t st{};
    st.firmware_version = 0xFFFF;
    this->update_state_(st);
    return TionsApi3s::write_state(st);
  }

  this->publish_state();
  this->dirty_ = true;
  this->parent_->set_enabled(true);
  return true;
}

}  // namespace tion
}  // namespace esphome
