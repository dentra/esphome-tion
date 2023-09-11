#include <cinttypes>

#include "esphome/core/log.h"
#include "tion_lt_climate.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_lt_climate";

void TionLtClimate::dump_config() {
  this->dump_settings(TAG, "Tion Lite");
  this->dump_presets(TAG);
}

void TionLtClimate::update_state(const tionlt_state_t &state) {
  this->dump_state(state);

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
    this->filter_warnout_->publish_state(state.flags.filter_warnout);
  }
  if (this->filter_time_left_) {
    this->filter_time_left_->publish_state(state.counters.filter_days());
  }
}

void TionLtClimate::dump_state(const tionlt_state_t &state) const {
  ESP_LOGV(TAG, "sound_state       : %s", ONOFF(state.flags.sound_state));
  ESP_LOGV(TAG, "led_state         : %s", ONOFF(state.flags.led_state));
  ESP_LOGV(TAG, "current_temp      : %d", state.current_temperature);
  ESP_LOGV(TAG, "outdoor_temp      : %d", state.outdoor_temperature);
  ESP_LOGV(TAG, "heater_power      : %f", state.heater_power());
  ESP_LOGV(TAG, "airflow_counter   : %f", state.counters.airflow_counter());
  ESP_LOGV(TAG, "filter_warnout    : %s", ONOFF(state.flags.filter_warnout));
  ESP_LOGV(TAG, "filter_time       : %" PRIu32, state.counters.filter_time);
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
  ESP_LOGV(TAG, "fan_time          : %" PRIu32, state.counters.fan_time);
  ESP_LOGV(TAG, "work_time         : %" PRIu32, state.counters.work_time);
  ESP_LOGV(TAG, "errors.reg        : %" PRIu32, state.errors.reg);
  ESP_LOGV(TAG, "errors.cnt        : %s",
           format_hex_pretty(reinterpret_cast<const uint8_t *>(&state.errors.cnt), sizeof(state.errors.cnt)).c_str());
  ESP_LOGV(TAG, "btn_prs.temp0     : %d", state.button_presets.temp[0]);
  ESP_LOGV(TAG, "btn_prs.fan_speed0: %d", state.button_presets.fan_speed[0]);
  ESP_LOGV(TAG, "btn_prs.temp1     : %d", state.button_presets.temp[1]);
  ESP_LOGV(TAG, "btn_prs.fan_speed1: %d", state.button_presets.fan_speed[1]);
  ESP_LOGV(TAG, "btn_prs.temp2     : %d", state.button_presets.temp[2]);
  ESP_LOGV(TAG, "btn_prs.fan_speed2: %d", state.button_presets.fan_speed[2]);
  ESP_LOGV(TAG, "test_type         : %u", state.test_type);
}

void TionLtClimate::control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature,
                                          bool buzzer, bool led) {
  if (mode == climate::CLIMATE_MODE_OFF) {
    this->control_state(false, this->state_.flags.heater_state, fan_speed, target_temperature, buzzer, led);
    return;
  }

  if (mode == climate::CLIMATE_MODE_HEAT_COOL) {
    this->control_state(true, this->state_.flags.heater_state, fan_speed, target_temperature, buzzer, led);
    return;
  }

  this->control_state(true, mode == climate::CLIMATE_MODE_HEAT, fan_speed, target_temperature, buzzer, led);
}

void TionLtClimate::control_state(bool power_state, bool heater_state, uint8_t fan_speed, int8_t target_temperature,
                                  bool buzzer, bool led) {
  tionlt_state_t st = this->state_;

  st.flags.power_state = power_state;
  if (this->state_.flags.power_state != st.flags.power_state) {
    ESP_LOGD(TAG, "New power state %s -> %s", ONOFF(this->state_.flags.power_state), ONOFF(st.flags.power_state));
  }

  st.flags.heater_state = heater_state;
  if (this->state_.flags.heater_state != st.flags.heater_state) {
    ESP_LOGD(TAG, "New heater state %s -> %s", ONOFF(this->state_.flags.heater_state), ONOFF(st.flags.heater_state));
  }

  st.fan_speed = fan_speed;
  if (this->state_.fan_speed != st.fan_speed) {
    ESP_LOGD(TAG, "New fan speed %u -> %u", this->state_.fan_speed, st.fan_speed);
  }

  st.target_temperature = target_temperature;
  if (this->state_.target_temperature != st.target_temperature) {
    ESP_LOGD(TAG, "New target temperature %d -> %d", this->state_.target_temperature, st.target_temperature);
  }

  st.flags.sound_state = buzzer;
  if (this->state_.flags.sound_state != st.flags.sound_state) {
    ESP_LOGD(TAG, "New sound state %s -> %s", ONOFF(this->state_.flags.sound_state), ONOFF(st.flags.sound_state));
  }

  st.flags.led_state = led;
  if (this->state_.flags.led_state != st.flags.led_state) {
    ESP_LOGD(TAG, "New led state %s -> %s", ONOFF(this->state_.flags.led_state), ONOFF(st.flags.led_state));
  }

  this->api_->write_state(st, ++this->request_id_);
}

}  // namespace tion
}  // namespace esphome
