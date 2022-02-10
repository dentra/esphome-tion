#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "tion_lt.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_lt";

void TionLt::on_ready() {
  //
  this->request_dev_status();
}

void TionLt::read(const tion_dev_status_t &status) {
  if (status.device_type != tion_dev_status_t::BRLT) {
    this->parent_->set_enabled(false);
    ESP_LOGE(TAG, "Unsupported device type %04X", status.device_type);
    return;
  }
  this->read_dev_status_(status);
  this->request_state();
};

void TionLt::read(const tionlt_state_t &state) {
  if (this->dirty_) {
    this->dirty_ = false;
    tionlt_state_t st = state;
    this->update_state_(st);
    TionApiLt::write_state(st);
    return;
  }

  this->max_fan_speed_ = state.limits.max_fan_speed;

  if (state.system.power_state) {
    this->mode = state.system.heater_state ? climate::CLIMATE_MODE_HEAT : climate::CLIMATE_MODE_FAN_ONLY;
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
  }

  this->target_temperature = state.system.target_temperature;

  this->set_fan_mode_(state.system.fan_speed);

  if (this->buzzer_) {
    this->buzzer_->publish_state(state.system.sound_state);
  } else {
    ESP_LOGV(TAG, "sound_state           : %s", ONOFF(state.system.sound_state));
  }
  if (this->led_) {
    this->led_->publish_state(state.system.led_state);
  } else {
    ESP_LOGV(TAG, "led_state             : %s", ONOFF(state.system.led_state));
  }
  if (this->temp_in_) {
    this->temp_in_->publish_state(state.sensors.indoor_temperature);
  } else {
    ESP_LOGV(TAG, "indoor_temperature    : %d", state.sensors.indoor_temperature);
  }
  if (this->temp_out_) {
    this->temp_out_->publish_state(state.sensors.outdoor_temperature);
  } else {
    ESP_LOGV(TAG, "outdoor_temperature   : %d", state.sensors.outdoor_temperature);
  }
  if (this->heater_power_) {
    this->heater_power_->publish_state(state.heater_power());
  } else {
    ESP_LOGV(TAG, "heater_power          : %f", state.heater_power());
  }
  if (this->airflow_counter_) {
    this->airflow_counter_->publish_state(state.counters.airflow_counter());
  } else {
    ESP_LOGV(TAG, "airflow_counter       : %f", state.counters.airflow_counter());
  }
  if (this->filter_warnout_) {
    this->filter_warnout_->publish_state(state.system.filter_wornout);
  } else {
    ESP_LOGV(TAG, "filter_wornout        : %s", ONOFF(state.system.filter_wornout));
  }
  if (this->filter_days_left_) {
    this->filter_days_left_->publish_state(state.counters.fileter_days());
  } else {
    ESP_LOGV(TAG, "counters.filter_time  : %u", state.counters.filter_time);
  }

  ESP_LOGV(TAG, "last_com_source       : %u", state.system.last_com_source);
  ESP_LOGV(TAG, "auto_co2              : %u", state.system.auto_co2);
  ESP_LOGV(TAG, "heater_state          : %u", state.system.heater_state);
  ESP_LOGV(TAG, "heater_present        : %u", state.system.heater_present);
  ESP_LOGV(TAG, "heater_var            : %u", state.heater_var);
  ESP_LOGV(TAG, "kiv_present           : %u", state.system.kiv_present);
  ESP_LOGV(TAG, "kiv_active            : %u", state.system.kiv_active);
  ESP_LOGV(TAG, "reserved              : %02x", state.system.reserved);
  ESP_LOGV(TAG, "gate_position         : %u", state.system.gate_position);
  ESP_LOGV(TAG, "pcb_temperature       : %u", state.sensors.pcb_temperature);
  ESP_LOGV(TAG, "fan_time              : %u", state.counters.fan_time);
  ESP_LOGV(TAG, "work_time             : %u", state.counters.work_time);
  ESP_LOGV(TAG, "errors.reg            : %u", state.errors.reg);
  ESP_LOGV(TAG, "errors.cnt            : %s",
           format_hex_pretty(reinterpret_cast<const uint8_t *>(&state.errors.cnt), sizeof(state.errors.cnt)).c_str());
  ESP_LOGV(TAG, "btn_presets.temp0     : %d", state.button_presets.temp[0]);
  ESP_LOGV(TAG, "btn_presets.fan_speed0: %d", state.button_presets.fan_speed[0]);
  ESP_LOGV(TAG, "btn_presets.temp1     : %d", state.button_presets.temp[1]);
  ESP_LOGV(TAG, "btn_presets.fan_speed1: %d", state.button_presets.fan_speed[1]);
  ESP_LOGV(TAG, "btn_presets.temp2     : %d", state.button_presets.temp[2]);
  ESP_LOGV(TAG, "btn_presets.fan_speed2: %d", state.button_presets.fan_speed[2]);
  ESP_LOGV(TAG, "test_type             : %u", state.test_type);

  this->publish_state();
  // leave 3 sec connection left for end all of jobs
  App.scheduler.set_timeout(this, TAG, 3000, [this]() { this->parent_->set_enabled(false); });
}

void TionLt::update_state_(tionlt_state_t &state) {
  if (this->custom_fan_mode.has_value()) {
    state.system.fan_speed = this->get_fan_speed();
  }

  state.system.target_temperature = this->target_temperature;
  state.system.power_state = this->mode != climate::CLIMATE_MODE_OFF;

  if (this->led_) {
    state.system.led_state = this->led_->state;
  }

  if (this->buzzer_) {
    state.system.sound_state = this->buzzer_->state;
  }

  state.system.heater_state = this->mode == climate::CLIMATE_MODE_HEAT ? 1 : 0;
}

}  // namespace tion
}  // namespace esphome
