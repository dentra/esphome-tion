#include "esphome/core/log.h"
#include "tion_lt.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_lt";

void TionLt::on_ready() { this->request_dev_status(); }

void TionLt::read(const tion_dev_status_t &status) {
  if (status.device_type != tion_dev_status_t::BRLT) {
    this->parent_->set_enabled(false);
    ESP_LOGE(TAG, "Unsupported device type %04X", status.device_type);
    return;
  }
  this->read_dev_status_(status);
  this->run_polling();
}

void TionLt::run_polling() {
  this->request_state();
  this->schedule_disconnect(this->state_timeout_);
}

void TionLt::read(const tionlt_state_t &state) {
  if (this->dirty_) {
    this->dirty_ = false;
    this->flush_state_(state);
    return;
  }

  this->cancel_disconnect();

  this->max_fan_speed_ = state.max_fan_speed;

  this->mode = state.flags.power_state
                   ? state.flags.heater_state ? climate::CLIMATE_MODE_HEAT : climate::CLIMATE_MODE_FAN_ONLY
                   : climate::CLIMATE_MODE_OFF;

  this->action = this->mode == climate::CLIMATE_MODE_OFF ? climate::CLIMATE_ACTION_OFF
                 : state.heater_var > 0                  ? climate::CLIMATE_ACTION_HEATING
                                                         : climate::CLIMATE_ACTION_FAN;

  this->current_temperature = state.current_temperature;
  this->target_temperature = state.target_temperature;
  this->set_fan_speed_(state.fan_speed);

  this->publish_state();

  if (this->buzzer_) {
    this->buzzer_->publish_state(state.flags.sound_state);
  }
  if (this->led_) {
    this->led_->publish_state(state.flags.led_state);
  }
  if (this->outdoor_temperature_) {
    this->outdoor_temperature_->publish_state(state.outdoor_temperature);
  }
  if (this->heater_power_) {
    this->heater_power_->publish_state(state.heater_power());
  }
  if (this->airflow_counter_) {
    this->airflow_counter_->publish_state(state.counters.airflow_counter());
  }
  if (this->filter_warnout_) {
    this->filter_warnout_->publish_state(state.flags.filter_wornout);
  }
  if (this->filter_days_left_) {
    this->filter_days_left_->publish_state(state.counters.filter_days());
  }

  ESP_LOGV(TAG, "sound_state       : %s", ONOFF(state.flags.sound_state));
  ESP_LOGV(TAG, "led_state         : %s", ONOFF(state.flags.led_state));
  ESP_LOGV(TAG, "current_temp      : %d", state.current_temperature);
  ESP_LOGV(TAG, "outdoor_temp      : %d", state.outdoor_temperature);
  ESP_LOGV(TAG, "heater_power      : %f", state.heater_power());
  ESP_LOGV(TAG, "airflow_counter   : %f", state.counters.airflow_counter());
  ESP_LOGV(TAG, "filter_wornout    : %s", ONOFF(state.flags.filter_wornout));
  ESP_LOGV(TAG, "filter_time       : %u", state.counters.filter_time);
  ESP_LOGV(TAG, "last_com_source   : %u", state.flags.last_com_source);
  ESP_LOGV(TAG, "auto_co2          : %s", ONOFF(state.flags.auto_co2));
  ESP_LOGV(TAG, "heater_state      : %s", ONOFF(state.flags.heater_state));
  ESP_LOGV(TAG, "heater_present    : %s", ONOFF(state.flags.heater_present));
  ESP_LOGV(TAG, "heater_var        : %u", state.heater_var);
  ESP_LOGV(TAG, "kiv_present       : %s", ONOFF(state.flags.kiv_present));
  ESP_LOGV(TAG, "kiv_active        : %s", ONOFF(state.flags.kiv_active));
  ESP_LOGV(TAG, "reserved          : %02x", state.flags.reserved);
  ESP_LOGV(TAG, "gate_position     : %u", state.gate_position);
  ESP_LOGV(TAG, "pcb_temperature   : %u", state.pcb_temperature);
  ESP_LOGV(TAG, "fan_time          : %u", state.counters.fan_time);
  ESP_LOGV(TAG, "work_time         : %u", state.counters.work_time);
  ESP_LOGV(TAG, "errors.reg        : %u", state.errors.reg);
  ESP_LOGV(TAG, "errors.cnt        : %s",
           format_hex_pretty(reinterpret_cast<const uint8_t *>(&state.errors.cnt), sizeof(state.errors.cnt)).c_str());
  ESP_LOGV(TAG, "btn_prs.temp0     : %d", state.button_presets.temp[0]);
  ESP_LOGV(TAG, "btn_prs.fan_speed0: %d", state.button_presets.fan_speed[0]);
  ESP_LOGV(TAG, "btn_prs.temp1     : %d", state.button_presets.temp[1]);
  ESP_LOGV(TAG, "btn_prs.fan_speed1: %d", state.button_presets.fan_speed[1]);
  ESP_LOGV(TAG, "btn_prs.temp2     : %d", state.button_presets.temp[2]);
  ESP_LOGV(TAG, "btn_prs.fan_speed2: %d", state.button_presets.fan_speed[2]);
  ESP_LOGV(TAG, "test_type         : %u", state.test_type);

  if (!this->is_persistent_connection()) {
    this->schedule_disconnect();
  }
}

void TionLt::flush_state_(const tionlt_state_t &state_) const {
  tionlt_state_t state = state_;
  if (this->custom_fan_mode.has_value()) {
    auto fan_speed = this->get_fan_speed_();
    if (state.fan_speed != fan_speed) {
      ESP_LOGD(TAG, "New fan speed %u", fan_speed);
      state.fan_speed = fan_speed;
    }
  }

  int8_t target_temperature = this->target_temperature;
  if (state.target_temperature != target_temperature) {
    ESP_LOGD(TAG, "New target temperature %d", target_temperature);
    state.target_temperature = target_temperature;
  }

  auto power_state = this->mode != climate::CLIMATE_MODE_OFF;
  if (state.flags.power_state != power_state) {
    ESP_LOGD(TAG, "New power state %s", ONOFF(power_state));
    state.flags.power_state = power_state;
  }

  if (this->led_) {
    auto led_state = this->led_->state;
    if (state.flags.led_state != led_state) {
      ESP_LOGD(TAG, "New led state %s", ONOFF(led_state));
      state.flags.led_state = led_state;
    }
  }

  if (this->buzzer_) {
    auto sound_state = this->buzzer_->state;
    if (state.flags.sound_state != sound_state) {
      ESP_LOGD(TAG, "New sound state %s", ONOFF(sound_state));
      state.flags.sound_state = sound_state;
    }
  }

  auto heater_state = this->mode == climate::CLIMATE_MODE_HEAT;
  if (state.flags.heater_state != heater_state) {
    ESP_LOGD(TAG, "New heater state %s", ONOFF(heater_state));
    state.flags.heater_state = heater_state;
  }

  TionApiLt::write_state(state);
}

bool TionLt::write_state() {
  this->publish_state();
  this->dirty_ = true;
  if (this->is_persistent_connection() && this->is_connected()) {
    this->request_state();
  } else {
    this->parent_->set_enabled(true);
  }
  return true;
}

void TionLt::update() {
  if (this->is_persistent_connection() && this->is_connected()) {
    this->run_polling();
  } else {
    this->parent_->set_enabled(true);
  }
}

}  // namespace tion
}  // namespace esphome
