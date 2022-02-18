#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/ble_client/ble_client.h"
#include <esp_bt_main.h>
#include <esp_bt.h>
#include "ble_secure.h"

namespace esphome {
namespace ble_secure {

static const char *const TAG = "ble_secure";

extern "C" void esp_profile_cb_reset(void);

static const char *esp_key_type_to_str(esp_ble_key_type_t key_type) {
  const char *key_str = nullptr;
  switch (key_type) {
    case ESP_LE_KEY_NONE:
      key_str = "ESP_LE_KEY_NONE";
      break;
    case ESP_LE_KEY_PENC:
      key_str = "ESP_LE_KEY_PENC";
      break;
    case ESP_LE_KEY_PID:
      key_str = "ESP_LE_KEY_PID";
      break;
    case ESP_LE_KEY_PCSRK:
      key_str = "ESP_LE_KEY_PCSRK";
      break;
    case ESP_LE_KEY_PLK:
      key_str = "ESP_LE_KEY_PLK";
      break;
    case ESP_LE_KEY_LLK:
      key_str = "ESP_LE_KEY_LLK";
      break;
    case ESP_LE_KEY_LENC:
      key_str = "ESP_LE_KEY_LENC";
      break;
    case ESP_LE_KEY_LID:
      key_str = "ESP_LE_KEY_LID";
      break;
    case ESP_LE_KEY_LCSRK:
      key_str = "ESP_LE_KEY_LCSRK";
      break;
    default:
      key_str = "INVALID BLE KEY TYPE";
      break;
  }
  return key_str;
}

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

void BLESecureTracker::gap_event_handler_(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
  // ESP_LOGV(TAG, "BLE GAP event: 0x%x", event);
  // esp32_ble_tracker::ESP32BLETracker::gap_event_handler(event, param);

  if (event == ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT) {
    if (param->local_privacy_cmpl.status != ESP_BT_STATUS_SUCCESS) {
      ESP_LOGE(TAG, "Enabling GAP config local privacy failed: %u", param->local_privacy_cmpl.status);
      //} else {
      // esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
      // if (scan_ret) {
      //   ESP_LOGE(TAG, "set scan params error, error code = %x", scan_ret);
      // }
    }
  } else if (event == ESP_GAP_BLE_SEC_REQ_EVT) {
    ESP_LOGD(TAG, "GAP BLE security request");
    // send the positive(true) security response to the peer device to accept the security request.
    //  If not accept the security request, should sent the security response with negative(false) accept value
    esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
  } else if (event == ESP_GAP_BLE_NC_REQ_EVT) {
    // The app will receive this evt when the IO has DisplayYesNO capability and the peer device IO also has
    // DisplayYesNo capability. show the passkey number to the user to confirm it with the number displayed by peer
    // deivce.
    ESP_LOGV(TAG, "ESP_GAP_BLE_NC_REQ_EVT, the passkey notify number: %u", param->ble_security.key_notif.passkey);
    esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true);
  } else if (event == ESP_GAP_BLE_PASSKEY_NOTIF_EVT) {
    /// the app will receive this evt when the IO has Output capability and the peer device IO has Input capability.
    /// show the passkey number to the user to input it in the peer deivce.
    ESP_LOGV(TAG, "The passkey notify number: %06u", param->ble_security.key_notif.passkey);
  } else if (event == ESP_GAP_BLE_KEY_EVT) {
    // shows the ble key info share with peer device to the user.
    ESP_LOGV(TAG, "key type = %s", esp_key_type_to_str(param->ble_security.ble_key.key_type));
  } else if (event == ESP_GAP_BLE_OOB_REQ_EVT) {
    ESP_LOGV(TAG, "GAP BLE OOB request");
    uint8_t tk[16] = {1};  // If you paired with OOB, both devices need to use the same tk
    esp_ble_oob_req_reply(param->ble_security.ble_req.bd_addr, tk, sizeof(tk));
  } else if (event == ESP_GAP_BLE_AUTH_CMPL_EVT) {
    ESP_LOGD(TAG, "GAP BLE auth complete");
    const esp_bd_addr_t &bd_addr = param->ble_security.auth_cmpl.bd_addr;
    ESP_LOGD("  ", "remote BD_ADDR: %08x%04x", (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
             (bd_addr[4] << 8) + bd_addr[5]);
    ESP_LOGD("  ", "address type = 0x%02x", param->ble_security.auth_cmpl.addr_type);
    ESP_LOGD("  ", "pair status = %s", YESNO(param->ble_security.auth_cmpl.success));
    if (param->ble_security.auth_cmpl.success) {
      ESP_LOGD("  ", "auth mode = %s", esp_auth_req_to_str(param->ble_security.auth_cmpl.auth_mode));
    } else {
      ESP_LOGD("  ", "fail reason = 0x%02x", param->ble_security.auth_cmpl.fail_reason);
    }
  }
}

void BLESecureTracker::gattc_event_handler_(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                            esp_ble_gattc_cb_param_t *param) {
  // ESP_LOGI(TAG, "BLE GATTC event: 0x%x", event);
  // esp32_ble_tracker::ESP32BLETracker::gattc_event_handler(event, gattc_if, param);

  if (event == ESP_GATTC_REG_EVT) {
    auto res = esp_ble_gap_config_local_privacy(true);
    ESP_LOGV(TAG, "Enabling GAP config local privacy: %s", TRUEFALSE(res == ESP_OK));

    // esp_ble_gap_set_device_name(EXAMPLE_DEVICE_NAME);
    // esp_ble_gap_config_local_icon(ESP_BLE_APPEARANCE_GENERIC_COMPUTER);
    return;
  }
}

void BLESecureTracker::secure_ble_setup_() {
  esp_err_t err;

  // stops esp32_ble_tracker already started scan
  // err = esp_ble_gap_stop_scanning();
  // ESP_LOGV(TAG, "Stop BLE scanning: %s", TRUEFALSE(err == ESP_OK));

  /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/

  esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;  // bonding with peer device after authentication
  err = esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(auth_req));
  ESP_LOGV(TAG, "Setting BLE_SM_AUTHEN_REQ_MODE: %s", TRUEFALSE(err == ESP_OK));

  // !!! io cap mode already set during ESP32BLETracker::ble_setup()
  // !!! do we need to setup it again?
  esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;  // set the IO capability to No output No input
  esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap));
  ESP_LOGV(TAG, "Setting ESP_BLE_SM_IOCAP_MODE: %s", TRUEFALSE(err == ESP_OK));

  uint8_t key_size = 16;  // the key size should be 7~16 bytes
  err = esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(key_size));
  ESP_LOGV(TAG, "Setting ESP_BLE_SM_MAX_KEY_SIZE: %s", TRUEFALSE(err == ESP_OK));

  uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  err = esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(init_key));
  ESP_LOGV(TAG, "Setting ESP_BLE_SM_SET_INIT_KEY: %s", TRUEFALSE(err == ESP_OK));

  uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  err = esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(rsp_key));
  ESP_LOGV(TAG, "Setting ESP_BLE_SM_SET_RSP_KEY: %s", TRUEFALSE(err == ESP_OK));

  // esp_profile_cb_reset();

  err = esp_ble_gap_register_callback(BLESecureTracker::gap_event_handler_);
  ESP_LOGV(TAG, "Reregistering GAP callback: %s", TRUEFALSE(err == ESP_OK));

  // err = esp_ble_gattc_register_callback(BLESecureTracker::gattc_event_handler_);
  // ESP_LOGV(TAG, "Registering GATTC callback: %s", TRUEFALSE(err == ESP_OK));

  auto bs = reinterpret_cast<BLESecureTracker *>(esp32_ble_tracker::global_esp32_ble_tracker);
  for (int i = 0, sz = bs->get_clients().size(); i < sz; i++) {
    auto client = new BLESecureClient(bs, bs->get_clients()[i]);
    bs->get_clients()[i] = client;
  }
}

float BLESecure::get_setup_priority() const {
  // we should setup after esp32_ble_tracker but before ble_client
  return (setup_priority::BLUETOOTH + setup_priority::AFTER_BLUETOOTH) / 2;
}

void BLESecure::setup() {
  // disconnection of all ble clients
  // for (auto bt_client : esp32_ble_tracker::global_esp32_ble_tracker->clients_) {
  //   auto client = reinterpret_cast<ble_client::BLEClient *>(bt_client);
  //   ESP_LOGV(TAG, "Disabling ble_client on startup: %s", client->address_str().c_str());
  //   client->set_enabled(false);
  // }
  BLESecureTracker::secure_ble_setup_();
}

void BLESecure::loop() {}

void BLESecureClient::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t esp_gattc_if,
                                          esp_ble_gattc_cb_param_t *param) {
  // ESP_LOGV(TAG, "BLESecureClient GATTC event: 0x%x", event);

  this->parent_client_->gattc_event_handler(event, esp_gattc_if, param);
  BLESecureTracker::gattc_event_handler_(event, esp_gattc_if, param);
}

}  // namespace ble_secure
}  // namespace esphome
