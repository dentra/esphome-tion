
#include <esp_gap_ble_api.h>
#include <esp_gattc_api.h>
#include <esp_bt_defs.h>
#include <esp_gatt_common_api.h>
#include "esp_bt.h"

extern "C" {

esp_err_t esp_ble_set_encryption(esp_bd_addr_t bd_addr, esp_ble_sec_act_t sec_act) { return ESP_OK; }

esp_err_t esp_ble_gattc_register_for_notify(esp_gatt_if_t gattc_if, esp_bd_addr_t server_bda, uint16_t handle) {
  return ESP_OK;
}
esp_err_t esp_ble_gattc_write_char(esp_gatt_if_t gattc_if, uint16_t conn_id, uint16_t handle, uint16_t value_len,
                                   uint8_t *value, esp_gatt_write_type_t write_type, esp_gatt_auth_req_t auth_req) {
  printf("esp_ble_gattc_write_char\n");
  return ESP_OK;
}
esp_err_t esp_ble_gattc_open(esp_gatt_if_t gattc_if, esp_bd_addr_t remote_bda, esp_ble_addr_type_t remote_addr_type,
                             bool is_direct) {
  return ESP_OK;
}
esp_err_t esp_ble_gattc_close(esp_gatt_if_t gattc_if, uint16_t conn_id) { return ESP_OK; }
esp_gatt_status_t esp_ble_gattc_get_all_descr(esp_gatt_if_t gattc_if, uint16_t conn_id, uint16_t char_handle,
                                              esp_gattc_descr_elem_t *result, uint16_t *count, uint16_t offset) {
  return ESP_GATT_OK;
}
esp_err_t esp_ble_gattc_app_register(uint16_t app_id) { return ESP_OK; }
esp_err_t esp_ble_gattc_search_service(esp_gatt_if_t gattc_if, uint16_t conn_id, esp_bt_uuid_t *filter_uuid) {
  return ESP_OK;
}
esp_err_t esp_ble_gattc_write_char_descr(esp_gatt_if_t gattc_if, uint16_t conn_id, uint16_t handle, uint16_t value_len,
                                         uint8_t *value, esp_gatt_write_type_t write_type,
                                         esp_gatt_auth_req_t auth_req) {
  return ESP_OK;
}
esp_err_t esp_ble_gattc_register_callback(esp_gattc_cb_t callback) { return ESP_OK; }
esp_err_t esp_ble_gattc_send_mtu_req(esp_gatt_if_t gattc_if, uint16_t conn_id) { return ESP_OK; }
esp_gatt_status_t esp_ble_gattc_get_all_char(esp_gatt_if_t gattc_if, uint16_t conn_id, uint16_t start_handle,
                                             uint16_t end_handle, esp_gattc_char_elem_t *result, uint16_t *count,
                                             uint16_t offset) {
  return ESP_GATT_OK;
}

esp_err_t esp_ble_gap_stop_scanning(void) { return ESP_OK; }
esp_err_t esp_ble_gap_start_scanning(uint32_t duration) { return ESP_OK; }
esp_err_t esp_ble_gap_set_scan_params(esp_ble_scan_params_t *scan_params) { return ESP_OK; }
const char *esp_err_to_name(esp_err_t code) { return "CLOAK_NO_IMPL"; }
esp_err_t esp_ble_gap_register_callback(esp_gap_ble_cb_t callback) { return ESP_OK; }
esp_err_t esp_ble_gap_set_device_name(const char *name) { return ESP_OK; }
esp_err_t esp_ble_gap_set_security_param(esp_ble_sm_param_t param_type, void *value, uint8_t len) { return ESP_OK; }
esp_err_t esp_ble_gap_security_rsp(esp_bd_addr_t bd_addr, bool accept) { return ESP_OK; }

esp_bt_controller_status_t esp_bt_controller_get_status() { return ESP_BT_CONTROLLER_STATUS_ENABLED; }
esp_err_t esp_bt_controller_init(esp_bt_controller_config_t *cfg) { return ESP_OK; }
esp_err_t esp_bt_controller_mem_release(esp_bt_mode_t mode) { return ESP_OK; }
esp_err_t esp_bt_controller_enable(esp_bt_mode_t mode) { return ESP_OK; }

esp_err_t esp_bluedroid_enable(void) { return ESP_OK; }
esp_err_t esp_bluedroid_init(void) { return ESP_OK; }
}
