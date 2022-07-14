#include "esphome/core/log.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#include "tion_3s.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_3s";

#define DEFAULT_BOOST_TIME 10

// application scheduler name
static const char *const ASH_BOOST = "tion_3s-boost";

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

void Tion3s::setup() {
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

  if (this->pair_state_ < 0) {
    bool res = TionsApi3s::pair();
    if (res) {
      this->pair_state_ = 1;
      this->rtc_.save(&this->pair_state_);
    }
    ESP_LOGD(TAG, "Pairing complete: %s", YESNO(res));
    this->schedule_disconnect();
    return;
  }

  if (this->exterimental_always_pair_) {
    TionsApi3s::pair();
  }

  this->run_polling();
}

void Tion3s::run_polling() {
  bool res = this->request_state();
  ESP_LOGV(TAG, "Request state result: %s", YESNO(res));
  this->schedule_disconnect(this->state_timeout_);
}

void Tion3s::read(const tion3s_state_t &state) {
  if (this->dirty_) {
    this->dirty_ = false;
    this->flush_state_(state);
    return;
  }

  this->cancel_disconnect();

  if (state.flags.power_state) {
    this->mode = state.flags.heater_state ? climate::CLIMATE_MODE_HEAT : climate::CLIMATE_MODE_FAN_ONLY;
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
  }
  this->current_temperature = state.current_temperature;
  this->target_temperature = state.target_temperature;
  this->set_fan_speed_(state.fan_speed);

  this->publish_state();

  if (this->version_) {
    this->version_->publish_state(str_snprintf("%04X", 4, state.firmware_version));
  }
  if (this->buzzer_) {
    this->buzzer_->publish_state(state.flags.sound_state);
  }
  if (this->outdoor_temperature_) {
    this->outdoor_temperature_->publish_state(state.outdoor_temperature);
  }
  if (this->filter_days_left_) {
    this->filter_days_left_->publish_state(state.filter_time);
  }
  if (this->air_intake_) {
    auto air_intake = this->air_intake_->at(state.gate_position);
    if (air_intake.has_value()) {
      this->air_intake_->publish_state(*air_intake);
    }
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
  ESP_LOGV(TAG, "unknown_temp : %d", state.unknown_temperature);
  ESP_LOGV(TAG, "outdoor_temp : %d", state.outdoor_temperature);
  ESP_LOGV(TAG, "current_temp : %d", state.current_temperature);
  ESP_LOGV(TAG, "filter_time  : %u", state.filter_time);
  ESP_LOGV(TAG, "hours        : %u", state.hours);
  ESP_LOGV(TAG, "minutes      : %u", state.minutes);
  ESP_LOGV(TAG, "last_error   : %u", state.last_error);
  ESP_LOGV(TAG, "productivity : %u", state.productivity);
  ESP_LOGV(TAG, "filter_days  : %u", state.filter_days);
  ESP_LOGV(TAG, "firmware     : %04X", state.firmware_version);

  if (!this->is_persistent_connection()) {
    this->schedule_disconnect();
  }
}

void Tion3s::flush_state_(const tion3s_state_t &state_) const {
  tion3s_state_t state = state_;
  if (this->custom_fan_mode.has_value()) {
    state.fan_speed = this->get_fan_speed_();
  }

  state.target_temperature = this->target_temperature;
  state.flags.power_state = this->mode != climate::CLIMATE_MODE_OFF;

  if (this->buzzer_) {
    state.flags.sound_state = this->buzzer_->state;
  }

  if (this->air_intake_) {
    auto air_intake = this->air_intake_->active_index();
    if (air_intake.has_value()) {
      if (*air_intake != state.gate_position) {
        ESP_LOGD(TAG, "New gate position %u", *air_intake);
        state.gate_position = *air_intake;
      }
    }
  }

  // режим вентиляция изменить на обогрев можно только через выключение
  if (state.flags.power_state && !state.flags.heater_state && this->mode == climate::CLIMATE_MODE_HEAT) {
    state.flags.power_state = false;
    TionsApi3s::write_state(state);
    state.flags.power_state = true;
  }

  state.flags.heater_state = this->mode == climate::CLIMATE_MODE_HEAT;

  TionsApi3s::write_state(state);
}

bool Tion3s::write_state() {
  this->publish_state();
  this->dirty_ = true;
  if (this->is_persistent_connection() && this->is_connected()) {
    this->request_state();
  } else {
    this->parent_->set_enabled(true);
  }
  return true;
}

void Tion3s::update() {
  if (this->is_persistent_connection() && this->is_connected()) {
    this->run_polling();
  } else if (this->pair_state_ > 0) {
    this->parent_->set_enabled(true);
  }
}

}  // namespace tion
}  // namespace esphome
