#include "esphome/core/log.h"
#include "esphome/core/application.h"

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

void Tion3s::on_ready() { this->request_state(); }

void Tion3s::read(const tion3s_state_t &state) {
  if (this->dirty_) {
    this->dirty_ = false;
    tion3s_state_t st = state;
    this->update_state_(st);
    TionsApi3s::write_state(st);
    return;
  }

  if (state.system.power_state) {
    this->mode = state.system.heater_state ? climate::CLIMATE_MODE_HEAT : climate::CLIMATE_MODE_FAN_ONLY;
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
    this->buzzer_->publish_state(state.system.sound_state);
  } else {
    ESP_LOGV(TAG, "sound_state           : %s", ONOFF(state.system.sound_state));
  }
  if (this->temp_in_) {
    this->temp_in_->publish_state(state.indoor_temperature);
  } else {
    ESP_LOGV(TAG, "indoor_temperature    : %d", state.indoor_temperature);
  }
  if (this->temp_out_) {
    this->temp_out_->publish_state(state.outdoor_temperature2);
  } else {
    ESP_LOGV(TAG, "outdoor_temperature   : %d", state.outdoor_temperature2);
  }
  if (this->filter_days_left_) {
    this->filter_days_left_->publish_state(state.filter_days);
  } else {
    ESP_LOGV(TAG, "filter_days           : %u", state.filter_days);
  }

  // leave 3 sec connection left for end all of jobs
  App.scheduler.set_timeout(this, TAG, 3000, [this]() { this->parent_->set_enabled(false); });
}

void Tion3s::update_state_(tion3s_state_t &state) {
  if (this->custom_fan_mode.has_value()) {
    state.fan_speed = this->get_fan_speed();
  }

  state.target_temperature = this->target_temperature;
  state.system.power_state = this->mode != climate::CLIMATE_MODE_OFF;
  state.system.heater_state = this->mode == climate::CLIMATE_MODE_HEAT;

  if (this->buzzer_) {
    state.system.sound_state = this->buzzer_->state;
  }
}

}  // namespace tion
}  // namespace esphome
