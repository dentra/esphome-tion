#include "esphome/core/log.h"
#include "esphome/core/defines.h"

#include "tion_3s_climate.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_3s";

void Tion3sClimate::dump_config() {
  this->dump_settings(TAG, "Tion 3S");
  LOG_SELECT("  ", "Air Intake", this->air_intake_);
  ESP_LOGCONFIG("  ", "OFF befor HEAT: "
#ifdef USE_TION_ENABLE_OFF_BEFORE_HEAT
                      "enabled"
#else
                      "disabled"
#endif
  );
  this->dump_presets(TAG);
}

void Tion3sClimate::update_state(const tion3s_state_t &state) {
  this->dump_state(state);

  this->mode = !state.flags.power_state   ? climate::CLIMATE_MODE_OFF
               : state.flags.heater_state ? climate::CLIMATE_MODE_HEAT
                                          : climate::CLIMATE_MODE_FAN_ONLY;

  // heating detection borrowed from:
  // https://github.com/TionAPI/tion_python/blob/master/tion_btle/tion.py#L177
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

  // TODO do tests and remove
  // additional request after state response
  // if (this->vport_type_ == TionVPortType::VPORT_UART && this->state_.firmware_version < 0x003C) {
  //   // call on next loop
  //   this->defer([this]() { this->api_->request_command4(); });
  // }
}

void Tion3sClimate::dump_state(const tion3s_state_t &state) const {
  ESP_LOGV(TAG, "fan_speed    : %u", state.fan_speed);
  ESP_LOGV(TAG, "gate_position: %u", state.gate_position);
  ESP_LOGV(TAG, "target_temp  : %u", state.target_temperature);
  ESP_LOGV(TAG, "heater_state : %s", ONOFF(state.flags.heater_state));
  ESP_LOGV(TAG, "power_state  : %s", ONOFF(state.flags.power_state));
  ESP_LOGV(TAG, "timer_state  : %s", ONOFF(state.flags.timer_state));
  ESP_LOGV(TAG, "sound_state  : %s", ONOFF(state.flags.sound_state));
  ESP_LOGV(TAG, "preset_state : %s", ONOFF(state.flags.preset_state));
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

void Tion3sClimate::control_gate_position(tion3s_state_t::GatePosition gate_position) const {
  climate::ClimateMode mode = this->mode;
  if (gate_position == tion3s_state_t::GatePosition::GATE_POSITION_INDOOR && this->mode == climate::CLIMATE_MODE_HEAT) {
    ESP_LOGW(TAG, "INDOOR gate position allow only FAN_ONLY mode");
    mode = climate::CLIMATE_MODE_FAN_ONLY;
  }
  this->control_climate_state(mode, this->get_fan_speed_(), this->target_temperature, this->get_buzzer_(),
                              gate_position);
}

void Tion3sClimate::control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature) {
  auto gate_position = this->get_gate_position_();
  if (mode == climate::CLIMATE_MODE_HEAT && gate_position == tion3s_state_t::GatePosition::GATE_POSITION_INDOOR) {
    ESP_LOGW(TAG, "HEAT mode allow only OUTDOOR gate position");
    gate_position = tion3s_state_t::GatePosition::GATE_POSITION_OUTDOOR;
  }
  this->control_climate_state(mode, fan_speed, target_temperature, this->get_buzzer_(), gate_position);
}

void Tion3sClimate::control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature,
                                          bool buzzer, tion3s_state_t::GatePosition gate_position) const {
  if (mode == climate::CLIMATE_MODE_OFF) {
    this->control_state(false, this->state_.flags.heater_state, fan_speed, target_temperature, buzzer, gate_position);
    return;
  }

  if (mode == climate::CLIMATE_MODE_HEAT_COOL) {
    this->control_state(true, this->state_.flags.heater_state, fan_speed, target_temperature, buzzer, gate_position);
    return;
  }

  this->control_state(true, mode == climate::CLIMATE_MODE_HEAT, fan_speed, target_temperature, buzzer, gate_position);
}

void Tion3sClimate::control_state(bool power_state, bool heater_state, uint8_t fan_speed, int8_t target_temperature,
                                  bool buzzer, tion3s_state_t::GatePosition gate_position) const {
  tion3s_state_t st = this->state_;

  st.flags.power_state = power_state;
  if (this->state_.flags.power_state != st.flags.power_state) {
    ESP_LOGD(TAG, "New power state %s -> %s", ONOFF(this->state_.flags.power_state), ONOFF(st.flags.power_state));
  }

  st.flags.heater_state = heater_state;
  if (this->state_.flags.heater_state != st.flags.heater_state) {
    ESP_LOGD(TAG, "New heater state %s -> %s", ONOFF(this->state_.flags.heater_state), ONOFF(st.flags.heater_state));
  }

  st.fan_speed = fan_speed;
  if (this->state_.fan_speed != fan_speed) {
    ESP_LOGD(TAG, "New fan speed %u -> %u", this->state_.fan_speed, st.fan_speed);
  }

  st.target_temperature = target_temperature;
  if (this->state_.target_temperature != st.target_temperature) {
    ESP_LOGD(TAG, "New target temperature %d -> %d", this->state_.target_temperature, target_temperature);
  }

  st.flags.sound_state = buzzer;
  if (this->state_.flags.sound_state != st.flags.sound_state) {
    ESP_LOGD(TAG, "New sound state %s -> %s", ONOFF(this->state_.flags.sound_state), ONOFF(st.flags.sound_state));
  }

  st.gate_position = gate_position;
  if (this->state_.gate_position != st.gate_position) {
    ESP_LOGD(TAG, "New gate position %u -> %u", this->state_.gate_position, st.gate_position);
  }

#ifdef USE_TION_ENABLE_OFF_BEFORE_HEAT
  // режим вентиляция изменить на обогрев можно только через выключение
  if (this->state_.flags.power_state && !this->state_.flags.heater_state && st.flags.heater_state) {
    st.flags.power_state = false;
    this->api_->write_state(st);
    st.flags.power_state = true;
  }
#endif

  this->api_->write_state(st);
}

}  // namespace tion
}  // namespace esphome
