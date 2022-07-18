#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "inttypes.h"
#include "tion.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion";

// boost time update interval
#define BOOST_TIME_UPDATE_INTERVAL_SEC 20

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
    ESP_LOGV(TAG, "Registring for notify complete");
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
      ESP_LOGV(TAG, "Register for notify 0x%x complete: %s", this->char_rx_, YESNO(err == ESP_OK));
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
  for (uint8_t i = 1, max = i + this->max_fan_speed_; i < max; i++) {
    traits.add_supported_custom_fan_mode(this->fan_speed_to_mode_(i));
  }
  if (!this->supported_presets_.empty()) {
    traits.set_supported_presets(this->supported_presets_);
    traits.add_supported_preset(climate::CLIMATE_PRESET_NONE);
  }
  return traits;
}

void TionClimate::control(const climate::ClimateCall &call) {
  bool preset_set = false;
  if (call.get_preset().has_value()) {
    const auto new_preset = *call.get_preset();
    const auto old_preset = *this->preset;
    if (new_preset != old_preset) {
      this->cancel_preset_(old_preset);
      preset_set = this->enable_preset_(new_preset);
    }
  }

  if (call.get_mode().has_value()) {
    if (preset_set) {
      ESP_LOGW(TAG, "%s preset enabled. Ignore change mode.",
               LOG_STR_ARG(climate::climate_preset_to_string(*this->preset)));
    } else {
      auto mode = *call.get_mode();
      switch (mode) {
        case climate::CLIMATE_MODE_OFF:
        case climate::CLIMATE_MODE_HEAT:
        case climate::CLIMATE_MODE_FAN_ONLY:
          this->mode = mode;
          ESP_LOGD(TAG, "Set mode %u", mode);
          break;
        default:
          ESP_LOGW(TAG, "Unsupported mode %d", mode);
          return;
      }
    }
  }

  if (call.get_custom_fan_mode().has_value()) {
    if (preset_set) {
      ESP_LOGW(TAG, "%s preset enabled. Ignore change fan speed.",
               LOG_STR_ARG(climate::climate_preset_to_string(*this->preset)));
    } else {
      this->set_fan_speed_(this->fan_mode_to_speed_(call.get_custom_fan_mode()));
      this->preset = climate::CLIMATE_PRESET_NONE;
    }
  }

  if (call.get_target_temperature().has_value()) {
    if (preset_set) {
      ESP_LOGW(TAG, "%s preset enabled. Ignore change target temperature.",
               LOG_STR_ARG(climate::climate_preset_to_string(*this->preset)));
    } else {
      ESP_LOGD(TAG, "Set target temperature %f", target_temperature);
      const auto target_temperature = *call.get_target_temperature();
      this->target_temperature = target_temperature;
      this->preset = climate::CLIMATE_PRESET_NONE;
    }
  }

  this->write_state();
}

void TionClimate::set_fan_speed_(uint8_t fan_speed) {
  if (fan_speed > 0 && fan_speed <= this->max_fan_speed_) {
    this->custom_fan_mode = this->fan_speed_to_mode_(fan_speed);
  } else {
    if (!(this->mode == climate::CLIMATE_MODE_OFF && fan_speed == 0)) {
      ESP_LOGW(TAG, "Unsupported fan speed %u (max: %u)", fan_speed, this->max_fan_speed_);
    }
  }
}

bool TionClimate::enable_preset_(climate::ClimatePreset preset) {
  ESP_LOGD(TAG, "Enable preset %s", LOG_STR_ARG(climate::climate_preset_to_string(preset)));
  if (preset == climate::CLIMATE_PRESET_BOOST) {
    if (!this->enable_boost_()) {
      return false;
    }
    this->saved_preset_ = *this->preset;
  }

  if (*this->preset == climate::CLIMATE_PRESET_NONE) {
    this->presets_[climate::CLIMATE_PRESET_NONE].mode = this->mode;
    this->presets_[climate::CLIMATE_PRESET_NONE].fan_speed = this->get_fan_speed_();
    this->presets_[climate::CLIMATE_PRESET_NONE].target_temperature = this->target_temperature;
  }

  this->mode = this->presets_[preset].mode;
  this->set_fan_speed_(this->presets_[preset].fan_speed);
  this->target_temperature = this->presets_[preset].target_temperature;
  this->preset = preset;

  return true;
}

void TionClimate::cancel_preset_(climate::ClimatePreset preset) {
  ESP_LOGD(TAG, "Cancel preset %s", LOG_STR_ARG(climate::climate_preset_to_string(preset)));
  if (preset == climate::CLIMATE_PRESET_BOOST) {
    this->cancel_boost_();
    this->enable_preset_(this->saved_preset_);
  }
}

void TionComponent::setup() {
  if (this->boost_time_ && std::isnan(this->boost_time_->state)) {
    auto call = this->boost_time_->make_call();
    call.set_value(DEFAULT_BOOST_TIME_SEC / 60);
    call.perform();
  }
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

void TionDisconnectMixinBase::schedule_disconnect_(TionComponent *c, ble_client::BLEClientNode *n, uint32_t timeout) {
  if (timeout) {
    App.scheduler.set_timeout(c, TAG, timeout, [n]() {
      ESP_LOGV(TAG, "Disconnecting");
      n->parent()->set_enabled(false);
    });
  }
}

void TionDisconnectMixinBase::cancel_disconnect_(TionComponent *c) { App.scheduler.cancel_timeout(c, TAG); }

bool TionClimateComponentWithBoost::enable_boost_() {
  uint32_t boost_time = this->boost_time_ ? this->boost_time_->state * 60 : DEFAULT_BOOST_TIME_SEC;
  if (boost_time == 0) {
    ESP_LOGW(TAG, "Boost time is not configured");
    return false;
  }

  // if boost_time_left not configured, just schedule stop boost after boost_time
  if (this->boost_time_left_ == nullptr) {
    ESP_LOGD(TAG, "Schedule boost timeout for %u s", boost_time);
    App.scheduler.set_timeout(this, ASH_BOOST, boost_time * 1000, [this]() {
      this->cancel_preset_(*this->preset);
      this->write_state();
    });
    return true;
  }

  // if boost_time_left is configured, schedule update it
  ESP_LOGD(TAG, "Schedule boost interval up to %u s", boost_time);
  App.scheduler.set_interval(this, ASH_BOOST, BOOST_TIME_UPDATE_INTERVAL_SEC * 1000, [this, boost_time]() {
    int time_left;
    if (std::isnan(this->boost_time_left_->state)) {
      time_left = boost_time;
    } else {
      time_left = (this->boost_time_left_->state * (boost_time / 100)) - BOOST_TIME_UPDATE_INTERVAL_SEC;
    }
    ESP_LOGV(TAG, "Boost time left %d s", time_left);
    if (time_left > 0) {
      this->boost_time_left_->publish_state(static_cast<float>(time_left) / static_cast<float>(boost_time / 100));
    } else {
      this->cancel_preset_(*this->preset);
      this->write_state();
    }
  });

  return true;
}

void TionClimateComponentWithBoost::cancel_boost_() {
  if (this->boost_time_left_) {
    ESP_LOGV(TAG, "Cancel boost interval");
    App.scheduler.cancel_interval(this, ASH_BOOST);
  } else {
    ESP_LOGV(TAG, "Cancel boost timeout");
    App.scheduler.cancel_timeout(this, ASH_BOOST);
  }
  this->boost_time_left_->publish_state(NAN);
}

}  // namespace tion
}  // namespace esphome
