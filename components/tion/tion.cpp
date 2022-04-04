#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "inttypes.h"
#include "tion.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion";

// default boost time - 10 minutes
#define DEFAULT_BOOST_TIME_SEC 10 * 60
// boost time update interval
#define BOOST_TIME_UPDATE_INTERVAL_SEC 30

// application scheduler name
static const char *const ASH_BOOST = "tion-boost";

// 98f00001-3788-83ea-453e-f52244709ddb
static const esp_bt_uuid_t BLE_TION_SERVICE{
    .len = ESP_UUID_LEN_128,
    .uuid = {
        .uuid128 = {0xDB, 0x9D, 0x70, 0x44, 0x22, 0xF5, 0x3E, 0x45, 0xEA, 0x83, 0x88, 0x37, 0x01, 0x00, 0xF0, 0x98},
    }};

// 98f00002-3788-83ea-453e-f52244709ddb
static const esp_bt_uuid_t BLE_TION_CHAR_TX{
    .len = ESP_UUID_LEN_128,
    .uuid = {
        .uuid128 = {0xDB, 0x9D, 0x70, 0x44, 0x22, 0xF5, 0x3E, 0x45, 0xEA, 0x83, 0x88, 0x37, 0x02, 0x00, 0xF0, 0x98},
    }};

// 98f00003-3788-83ea-453e-f52244709ddb
static const esp_bt_uuid_t BLE_TION_CHAR_RX{
    .len = ESP_UUID_LEN_128,
    .uuid = {
        .uuid128 = {0xDB, 0x9D, 0x70, 0x44, 0x22, 0xF5, 0x3E, 0x45, 0xEA, 0x83, 0x88, 0x37, 0x03, 0x00, 0xF0, 0x98},
    }};

void TionBleNode::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                      esp_ble_gattc_cb_param_t *param) {
  if (event == ESP_GATTC_NOTIFY_EVT) {
    ESP_LOGV(TAG, "Got notify for handle %04u: %s", param->notify.handle,
             format_hex_pretty(param->notify.value, param->notify.value_len).c_str());
    if (param->notify.handle == this->char_rx_) {
      this->read_data(param->notify.value, param->notify.value_len);
    }
    return;
  }

  if (event == ESP_GATTC_WRITE_DESCR_EVT) {
    ESP_LOGV(TAG, "write_char_descr at 0x%x complete 0x%02x", param->write.handle, param->write.status);
    this->node_state = esp32_ble_tracker::ClientState::ESTABLISHED;
    this->on_ready();
    return;
  }

  if (event == ESP_GATTC_REG_FOR_NOTIFY_EVT) {
    ESP_LOGD(TAG, "Registring for notify complete");
    return;
  }

  if (event == ESP_GATTC_SEARCH_CMPL_EVT) {
    auto ble_service = esp32_ble_tracker::ESPBTUUID::from_uuid(this->get_ble_service());

    auto ble_char_tx = esp32_ble_tracker::ESPBTUUID::from_uuid(this->get_ble_char_tx());
    auto tx = this->parent()->get_characteristic(ble_service, ble_char_tx);
    if (tx == nullptr) {
      ESP_LOGE(TAG, "Can't discover TX characteristics");
      return;
    }
    this->char_tx_ = tx->handle;

    auto ble_char_rx = esp32_ble_tracker::ESPBTUUID::from_uuid(this->get_ble_char_rx());
    auto rx = this->parent()->get_characteristic(ble_service, ble_char_rx);
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
      ESP_LOGV(TAG, "Register for notify  0x%x complete: %s", this->char_rx_, YESNO(err == ESP_OK));
    } else {
      this->node_state = esp32_ble_tracker::ClientState::ESTABLISHED;
      this->on_ready();
    }

    return;
  }

  if (event == ESP_GATTC_CONNECT_EVT) {
    // let device to pair
    auto ble_sec_act = this->get_ble_encryption();
    if (ble_sec_act != 0) {
      auto err = esp_ble_set_encryption(param->connect.remote_bda, ble_sec_act);
      ESP_LOGD(TAG, "Bonding complete: %s", YESNO(err == ESP_OK));
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

bool TionBleNode::write_data(const uint8_t *data, uint16_t size) const {
  ESP_LOGV(TAG, "write_data to 0x%x: %s", this->char_tx_, format_hex_pretty(data, size).c_str());
#ifdef ESPHOME_LOG_HAS_VERBOSE
  esp_gatt_write_type_t write_type = ESP_GATT_WRITE_TYPE_RSP;
#else
  esp_gatt_write_type_t write_type = ESP_GATT_WRITE_TYPE_NO_RSP;
#endif
  return esp_ble_gattc_write_char(this->parent_->gattc_if, this->parent_->conn_id, this->char_tx_, size,
                                  const_cast<uint8_t *>(data), write_type, ESP_GATT_AUTH_REQ_NONE) == ESP_OK;
}

const esp_bt_uuid_t &TionBase::get_ble_service() const { return BLE_TION_SERVICE; }

const esp_bt_uuid_t &TionBase::get_ble_char_tx() const { return BLE_TION_CHAR_TX; }

const esp_bt_uuid_t &TionBase::get_ble_char_rx() const { return BLE_TION_CHAR_RX; }

climate::ClimateTraits TionClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_visual_min_temperature(0.0);
  traits.set_visual_max_temperature(25.0);
  traits.set_visual_temperature_step(1.0);
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_FAN_ONLY,
  });
  for (char i = '1', max = i + this->max_fan_speed_; i < max; i++) {
    char buf[2] = {i, 0};
    traits.add_supported_custom_fan_mode(buf);
  }
  traits.set_supported_presets({
      climate_CLIMATE_PRESET_DEFAULT,
      climate::CLIMATE_PRESET_BOOST,
  });
  return traits;
}

void TionClimate::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value()) {
    auto mode = *call.get_mode();
    switch (mode) {
      case climate::CLIMATE_MODE_OFF:
      case climate::CLIMATE_MODE_HEAT:
      case climate::CLIMATE_MODE_FAN_ONLY:
        this->mode = mode;
        ESP_LOGD(TAG, "Set mode %u", mode);
        break;
      default:
        ESP_LOGW(TAG, "Unsupported mode: %d", mode);
        return;
    }
  }

  if (call.get_preset().has_value() && *call.get_preset() != *this->preset) {
    const auto preset = *call.get_preset();
    if (preset == climate::CLIMATE_PRESET_BOOST) {
      this->enable_boost();
    } else {
      this->cancel_boost();
    }
  }

  if (call.get_fan_mode().has_value()) {
    auto fan_mode = *call.get_fan_mode();
    ESP_LOGW(TAG, "Unsupported fan mode: %d", fan_mode);
  }

  if (call.get_custom_fan_mode().has_value()) {
    if (this->preset == climate::CLIMATE_PRESET_BOOST) {
      ESP_LOGW(TAG, "Boost preset enabled. Ignore change fan speed.");
    } else {
      const auto fan_mode = *call.get_custom_fan_mode();
      const auto fan_speed = *fan_mode.c_str() - '0';
      if (fan_speed <= this->max_fan_speed_) {
        this->custom_fan_mode = fan_mode;
        ESP_LOGD(TAG, "Set fan mode %s", fan_mode.c_str());
      } else {
        ESP_LOGW(TAG, "Unsupported fan mode: %u from %u", fan_speed, this->max_fan_speed_);
      }
    }
  }

  if (call.get_target_temperature().has_value()) {
    const auto target_temperature = *call.get_target_temperature();
    this->target_temperature = target_temperature;
    ESP_LOGD(TAG, "Set target temperature %f", target_temperature);
  }

  this->write_state();
}

void TionClimate::set_fan_speed(uint8_t fan_speed) {
  char fan_mode[2] = {static_cast<char>(fan_speed + '0'), 0};
  this->custom_fan_mode = std::string(fan_mode);
}

uint8_t TionClimate::get_fan_speed() const {
  if (this->custom_fan_mode.has_value()) {
    return *this->custom_fan_mode.value().c_str() - '0';
  }
  return 0;
}

void TionComponent::read_dev_status_(const dentra::tion::tion_dev_status_t &status) {
  if (this->version_ != nullptr) {
    this->version_->publish_state(str_snprintf("%04X", 4, status.firmware_version));
  }
  ESP_LOGV(TAG, "Work Mode       : %02X", status.work_mode);
  ESP_LOGV(TAG, "Device type     : %04X", status.device_type);
  ESP_LOGV(TAG, "Device sub-type : %04X", status.device_subtype);
  ESP_LOGV(TAG, "Hardware version: %04X", status.hardware_version);
  ESP_LOGV(TAG, "Firmware version: %04X", status.firmware_version);
}

void TionClimateComponentWithBoost::setup() { this->preset = climate_CLIMATE_PRESET_DEFAULT; }

void TionClimateComponentWithBoost::enable_boost() {
  uint32_t boost_time = this->boost_time_ ? this->boost_time_->state : DEFAULT_BOOST_TIME_SEC;
  if (boost_time == 0) {
    ESP_LOGW(TAG, "Boost time is not configured");
    return ;
  }

  if (*this->preset != climate::CLIMATE_PRESET_BOOST) {
    this->saved_preset_ = *this->preset;
    this->saved_fan_speed_ = this->get_fan_speed();
    this->preset = climate::CLIMATE_PRESET_BOOST;
  }
  this->set_fan_speed(this->max_fan_speed_);

  // if boost_time_left not configured, just schedule stop boost after boost_time
  if (this->boost_time_left_ == nullptr) {
    App.scheduler.set_timeout(this, ASH_BOOST, boost_time * 1000, [this]() {
      this->cancel_boost();
      this->write_state();
    });
    return ;
  }

  // if boost_time_left is configured, schedule update it
  App.scheduler.set_interval(this, ASH_BOOST, BOOST_TIME_UPDATE_INTERVAL_SEC * 1000, [this]() {
    int time_left = this->boost_time_left_->state;
    time_left -= BOOST_TIME_UPDATE_INTERVAL_SEC;
    if (time_left > 0) {
      this->boost_time_left_->publish_state(time_left);
    } else {
      this->cancel_boost();
      this->write_state();
    }
  });

  this->boost_time_left_->publish_state(boost_time);

  return ;
}

void TionClimateComponentWithBoost::cancel_boost() {
  if (this->boost_time_left_) {
    App.scheduler.cancel_interval(this, ASH_BOOST);
  } else {
    App.scheduler.cancel_timeout(this, ASH_BOOST);
  }
  this->set_fan_speed(this->saved_fan_speed_);
  this->boost_time_left_->publish_state(NAN);
  this->preset = this->saved_preset_;
}

}  // namespace tion
}  // namespace esphome
