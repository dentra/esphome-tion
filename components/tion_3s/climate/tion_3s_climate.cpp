#include "esphome/core/log.h"
#include "esphome/core/defines.h"

#include "tion_3s_climate.h"

#ifdef TION_ENABLE_OFF_BEFORE_HEAT
#define TION_OPTION_STR_OFF_BEFORE_HEAT "enabled"
#else
#define TION_OPTION_STR_OFF_BEFORE_HEAT "disabled"
#endif

#ifdef TION_ENABLE_ANTIFRIZE
#define TION_OPTION_STR_ANTIFRIZE "enabled"
#else
#define TION_OPTION_STR_ANTIFRIZE "disabled"
#endif

namespace esphome {
namespace tion {

static const char *const TAG = "tion_3s";

void Tion3sClimate::dump_config() {
  this->dump_settings(TAG, "Tion 3S");
  LOG_SELECT("  ", "Air Intake", this->air_intake_);
  ESP_LOGCONFIG("  ", "OFF befor HEAT: %s", TION_OPTION_STR_OFF_BEFORE_HEAT);
  ESP_LOGCONFIG("  ", "Antifrize: %s", TION_OPTION_STR_ANTIFRIZE);
  this->dump_presets(TAG);
}

void Tion3sClimate::update_state(const tion3s_state_t &state) {
  this->dump_state(state);

  if (!state.flags.power_state) {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->action = climate::CLIMATE_ACTION_OFF;
  } else if (state.flags.heater_state) {
    this->mode = climate::CLIMATE_MODE_HEAT;
    this->action = this->mode == state.is_heating() ? climate::CLIMATE_ACTION_HEATING : climate::CLIMATE_ACTION_FAN;
  } else {
    this->mode = climate::CLIMATE_MODE_FAN_ONLY;
    this->action = climate::CLIMATE_ACTION_FAN;
  }

  this->current_temperature = state.current_temperature();
  this->target_temperature = state.target_temperature;
  this->set_fan_speed_(state.fan_speed);
  this->publish_state();

  if (this->version_ && state.firmware_version > 0) {
    this->version_->publish_state(str_snprintf("%04X", 4, state.firmware_version));
  }
  if (this->air_intake_) {
    auto air_intake = this->air_intake_->at(state.gate_position);
    if (air_intake.has_value()) {
      this->air_intake_->publish_state(*air_intake);
    }
  }
  if (this->productivity_) {
    this->productivity_->publish_state(state.productivity);
  }
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
  ESP_LOGV(TAG, "current_temp1: %d", state.current_temperature1);
  ESP_LOGV(TAG, "current_temp2: %d", state.current_temperature2);
  ESP_LOGV(TAG, "outdoor_temp : %d", state.outdoor_temperature);
  ESP_LOGV(TAG, "filter_time  : %u", state.counters.filter_time);
  ESP_LOGV(TAG, "hours        : %u", state.hours);
  ESP_LOGV(TAG, "minutes      : %u", state.minutes);
  ESP_LOGV(TAG, "last_error   : %u", state.last_error);
  ESP_LOGV(TAG, "productivity : %u", state.productivity);
  ESP_LOGV(TAG, "filter_days  : %u", state.filter_days);
  ESP_LOGV(TAG, "firmware     : %04X", state.firmware_version);
}

void Tion3sClimate::control_gate_position(tion3s_state_t::GatePosition gate_position) {
  ControlState control{};
  control.gate_position = gate_position;

  if (gate_position == tion3s_state_t::GatePosition::GATE_POSITION_INDOOR) {
    // TODO необходимо проверять batch_active
    if (this->mode == climate::CLIMATE_MODE_HEAT) {
      ESP_LOGW(TAG, "INDOOR gate position allow only FAN_ONLY mode");
      control.heater_state = false;
    }
  }

  this->control_state_(control);
}

void Tion3sClimate::control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature,
                                          TionClimateGatePosition gate_position) {
  ControlState control{};
  control.fan_speed = fan_speed;
  control.target_temperature = target_temperature;

  if (mode == climate::CLIMATE_MODE_OFF) {
    control.power_state = false;
  } else if (mode == climate::CLIMATE_MODE_HEAT_COOL) {
    control.power_state = true;
  } else {
    control.power_state = true;
    control.heater_state = mode == climate::CLIMATE_MODE_HEAT;
  }

  switch (gate_position) {
    case TION_CLIMATE_GATE_POSITION_OUTDOOR:
      control.gate_position = tion3s_state_t::GatePosition::GATE_POSITION_OUTDOOR;
      break;
    case TION_CLIMATE_GATE_POSITION_INDOOR:
      control.gate_position = tion3s_state_t::GatePosition::GATE_POSITION_INDOOR;
      break;
    case TION_CLIMATE_GATE_POSITION_MIXED:
      control.gate_position = tion3s_state_t::GatePosition::GATE_POSITION_MIXED;
      break;
    default:
      control.gate_position = this->get_gate_position_();
      break;
  }

  if (mode == climate::CLIMATE_MODE_HEAT) {
    if (control.gate_position == tion3s_state_t::GatePosition::GATE_POSITION_INDOOR) {
      ESP_LOGW(TAG, "HEAT mode allow only OUTDOOR gate position");
      control.gate_position = tion3s_state_t::GatePosition::GATE_POSITION_OUTDOOR;
    }
  }

  this->control_state_(control);
}

void Tion3sClimate::control_state_(const ControlState &state) {
  tion3s_state_t st = this->state_;

  if (state.power_state.has_value()) {
    st.flags.power_state = *state.power_state;
    if (this->state_.flags.power_state != st.flags.power_state) {
      ESP_LOGD(TAG, "New power state %s -> %s", ONOFF(this->state_.flags.power_state), ONOFF(st.flags.power_state));
    }
  }

  if (state.heater_state.has_value()) {
    st.flags.heater_state = *state.heater_state;
    if (this->state_.flags.heater_state != st.flags.heater_state) {
      ESP_LOGD(TAG, "New heater state %s -> %s", ONOFF(this->state_.flags.heater_state), ONOFF(st.flags.heater_state));
    }
  }

  if (state.fan_speed.has_value()) {
    st.fan_speed = *state.fan_speed;
    if (this->state_.fan_speed != st.fan_speed) {
      ESP_LOGD(TAG, "New fan speed %u -> %u", this->state_.fan_speed, st.fan_speed);
    }
  }

  if (state.target_temperature.has_value()) {
    st.target_temperature = *state.target_temperature;
    if (this->state_.target_temperature != st.target_temperature) {
      ESP_LOGD(TAG, "New target temperature %d -> %d", this->state_.target_temperature, st.target_temperature);
    }
  }

  if (state.buzzer.has_value()) {
    st.flags.sound_state = *state.buzzer;
    if (this->state_.flags.sound_state != st.flags.sound_state) {
      ESP_LOGD(TAG, "New sound state %s -> %s", ONOFF(this->state_.flags.sound_state), ONOFF(st.flags.sound_state));
    }
  }

  if (state.gate_position.has_value()) {
    st.gate_position = *state.gate_position;
    if (this->state_.gate_position != st.gate_position) {
      ESP_LOGD(TAG, "New gate position %u -> %u", this->state_.gate_position, st.gate_position);
    }
  }

#ifdef TION_ENABLE_ANTIFRIZE
  if (st.flags.power_state && !st.flags.heater_state && this->outdoor_temperature_) {
    auto outdoor_temperature = this->outdoor_temperature_->state;
    if (!std::isnan(outdoor_temperature) && outdoor_temperature < 0.001) {
      ESP_LOGW(TAG, "Antifrize protection has worked. Heater now enabled.");
      st.flags.heater_state = true;
    }
  }
#endif

  // FIXME с батч режимом невозможно
  // #ifdef TION_ENABLE_OFF_BEFORE_HEAT
  //   // режим вентиляция изменить на обогрев можно только через выключение
  //   if (this->state_.flags.power_state && !this->state_.flags.heater_state && st.flags.heater_state) {
  //     st.flags.power_state = false;
  //     this->write_api_state_(st);
  //     st.flags.power_state = true;
  //   }
  // #endif

  this->write_api_state_(st);
}

}  // namespace tion
}  // namespace esphome
