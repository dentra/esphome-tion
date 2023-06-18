#include "esphome/core/log.h"

#include "tion_3s.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_3s";

void Tion3s::dump_config() {
  this->dump_settings(TAG, "Tion 3S");
  LOG_SELECT("  ", "Air Intake", this->air_intake_);
}

void Tion3s::update_state() {
  const auto &state = this->state_;

  this->mode = !state.flags.power_state   ? climate::CLIMATE_MODE_OFF
               : state.flags.heater_state ? climate::CLIMATE_MODE_HEAT
                                          : climate::CLIMATE_MODE_FAN_ONLY;

  bool is_heating = (state.target_temperature - state.outdoor_temperature) > 3 &&
                    state.current_temperature > state.outdoor_temperature;
  this->action = this->mode == climate::CLIMATE_MODE_OFF ? climate::CLIMATE_ACTION_OFF
                 : is_heating                            ? climate::CLIMATE_ACTION_HEATING
                                                         : climate::CLIMATE_ACTION_FAN;

  this->current_temperature = state.current_temperature;
  this->target_temperature = state.target_temperature;
  this->set_fan_speed_(state.fan_speed);
  this->publish_state();

  if (this->version_ && state.firmware_version > 0) {
    this->version_->publish_state(str_snprintf("%04X", 4, state.firmware_version));
  }
  if (this->buzzer_) {
    this->buzzer_->publish_state(state.flags.sound_state);
  }
  if (this->outdoor_temperature_) {
    this->outdoor_temperature_->publish_state(state.outdoor_temperature);
  }
  if (this->filter_time_left_) {
    this->filter_time_left_->publish_state(state.filter_time);
  }
  if (this->air_intake_) {
    auto air_intake = this->air_intake_->at(state.gate_position);
    if (air_intake.has_value()) {
      this->air_intake_->publish_state(*air_intake);
    }
  }
  if (this->airflow_counter_) {
    this->airflow_counter_->publish_state(state.productivity);
  }
}

void Tion3s::dump_state() const {
  const auto &state = this->state_;
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
}

void Tion3s::flush_state() {
  auto &state = this->state_;

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
    state.flags.power_state = this->mode != climate::CLIMATE_MODE_OFF;
  }

  if (this->buzzer_) {
    auto sound_state = this->buzzer_->state;
    if (state.flags.sound_state != sound_state) {
      ESP_LOGD(TAG, "New sound state %s", ONOFF(sound_state));
      state.flags.sound_state = this->buzzer_->state;
    }
  }

  if (this->air_intake_) {
    auto air_intake = this->air_intake_->active_index();
    if (air_intake.has_value() && state.gate_position != *air_intake) {
      ESP_LOGD(TAG, "New gate position %u", *air_intake);
      state.gate_position = *air_intake;
    }
  }

  auto heater_state = this->mode == climate::CLIMATE_MODE_HEAT;
  if (state.flags.heater_state != heater_state) {
    ESP_LOGD(TAG, "New heater state %s", ONOFF(heater_state));

    // режим вентиляция изменить на обогрев можно только через выключение
    if (state.flags.power_state && !state.flags.heater_state && heater_state) {
      state.flags.power_state = false;
      this->api_->write_state(state);
      state.flags.power_state = true;
    }

    state.flags.heater_state = heater_state;
  }

  this->api_->write_state(state);
}

}  // namespace tion
}  // namespace esphome
