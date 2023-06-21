#include "esphome/core/log.h"

#include <cstddef>
#include <ctime>
#include <cmath>

#include "tion_4s.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_4s";

void Tion4s::dump_config() {
  this->dump_settings(TAG, "Tion 4S");
  LOG_SWITCH("  ", "Recirculation", this->recirculation_);
}

#ifdef TION_ENABLE_SCHEDULER
void Tion4s::on_time(const time_t time, const uint32_t request_id) {
  auto c_tm = std::gmtime(&time);
  char buf[20] = {};
  std::strftime(buf, sizeof(buf), "%F %T", c_tm);
  ESP_LOGD(TAG, "Device UTC time: %s", buf);
  c_tm = std::localtime(&time);
  std::strftime(buf, sizeof(buf), "%F %T", c_tm);
  ESP_LOGD(TAG, "Device local time: %s", buf);
}
#endif

#ifdef TION_ENABLE_PRESETS
void Tion4s::on_turbo(const tion4s_turbo_t &turbo, const uint32_t request_id) {
  if (this->vport_type_ != TionVPortType::VPORT_BLE) {
    ESP_LOGW(TAG, "Only BLE supports native turbo mode. Please report.");
    return;
  }

  // change preset if turbo changed outside
  if (turbo.is_active) {
    if (*this->preset != climate::CLIMATE_PRESET_BOOST) {
      this->saved_preset_ = *this->preset;
      this->preset = climate::CLIMATE_PRESET_BOOST;
    }
  } else {
    if (*this->preset == climate::CLIMATE_PRESET_BOOST) {
      this->enable_preset_(this->saved_preset_);
    }
  }

  if (this->boost_time_left_) {
    if (turbo.turbo_time == 0) {
      this->boost_time_left_->publish_state(NAN);
    } else {
      auto boost_time = this->get_boost_time();
      this->boost_time_left_->publish_state(static_cast<float>(turbo.turbo_time) /
                                            static_cast<float>(boost_time / 100));
    }
  }

  this->publish_state();
}
#endif

void Tion4s::update_state(const tion4s_state_t &state) {
  this->dump_state(state);

  this->max_fan_speed_ = state.max_fan_speed;

  this->mode = state.flags.power_state ? state.flags.heater_mode == tion4s_state_t::HEATER_MODE_HEATING
                                             ? climate::CLIMATE_MODE_HEAT
                                             : climate::CLIMATE_MODE_FAN_ONLY
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
  if (this->recirculation_) {
    this->recirculation_->publish_state(state.gate_position == tion4s_state_t::GATE_POSITION_RECIRCULATION);
  }
}

void Tion4s::dump_state(const tion4s_state_t &state) const {
  ESP_LOGV(TAG, "sound_state    : %s", ONOFF(state.flags.sound_state));
  ESP_LOGV(TAG, "led_state      : %s", ONOFF(state.flags.led_state));
  ESP_LOGV(TAG, "current_temp   : %d", state.current_temperature);
  ESP_LOGV(TAG, "outdoor_temp   : %d", state.outdoor_temperature);
  ESP_LOGV(TAG, "heater_power   : %f", state.heater_power());
  ESP_LOGV(TAG, "airflow_counter: %f", state.counters.airflow_counter());
  ESP_LOGV(TAG, "filter_warnout : %s", ONOFF(state.flags.filter_warnout));
  ESP_LOGV(TAG, "filter_time    : %u", state.counters.filter_time);
  ESP_LOGV(TAG, "gate_position  : %u", state.gate_position);

  ESP_LOGV(TAG, "pcb_pwr_temp   : %d", state.pcb_pwr_temperature);
  ESP_LOGV(TAG, "pcb_ctl_temp   : %d", state.pcb_ctl_temperature);
  ESP_LOGV(TAG, "fan_speed      : %u", state.fan_speed);
  ESP_LOGV(TAG, "heater_mode    : %u", state.flags.heater_mode);
  ESP_LOGV(TAG, "heater_state   : %s", ONOFF(state.flags.heater_state));
  ESP_LOGV(TAG, "heater_present : %u", state.flags.heater_present);
  ESP_LOGV(TAG, "heater_var     : %u", state.heater_var);
  ESP_LOGV(TAG, "last_com_source: %u", state.flags.last_com_source);

  ESP_LOGV(TAG, "ma             : %s", ONOFF(state.flags.ma));
  ESP_LOGV(TAG, "ma_auto        : %s", ONOFF(state.flags.ma_auto));
  ESP_LOGV(TAG, "active_timer   : %s", ONOFF(state.flags.active_timer));
  ESP_LOGV(TAG, "reserved       : %02X", state.flags.reserved);
  ESP_LOGV(TAG, "work_time      : %u", state.counters.work_time);
  ESP_LOGV(TAG, "fan_time       : %u", state.counters.fan_time);
  ESP_LOGV(TAG, "errors         : %u", state.errors);
}

void Tion4s::control_state(climate::ClimateMode mode, uint8_t fan_speed, int8_t target_temperature, bool buzzer,
                           bool led, tion4s_state_t::GatePosition gate_position) const {
  tion4s_state_t st = this->state_;

  st.flags.power_state = mode != climate::CLIMATE_MODE_OFF;
  if (this->state_.flags.power_state != st.flags.power_state) {
    ESP_LOGD(TAG, "New power state %s -> %s", ONOFF(this->state_.flags.power_state), ONOFF(st.flags.power_state));
  }

  st.flags.heater_mode = mode == climate::CLIMATE_MODE_HEAT ? tion4s_state_t::HEATER_MODE_HEATING
                                                            : tion4s_state_t::HEATER_MODE_TEMPERATURE_MAINTENANCE;
  if (this->state_.flags.heater_mode != st.flags.heater_mode) {
    ESP_LOGD(TAG, "New heater mode %s -> %s", ONOFF(this->state_.flags.heater_mode), ONOFF(st.flags.heater_mode));
  }

  st.fan_speed = fan_speed;
  if (this->state_.fan_speed != st.fan_speed) {
    ESP_LOGD(TAG, "New fan speed %u -> %u", this->state_.fan_speed, st.fan_speed);
  }

  st.target_temperature = target_temperature;
  if (this->state_.target_temperature != st.target_temperature) {
    ESP_LOGD(TAG, "New target temperature %d -> %d", this->state_.target_temperature, st.target_temperature);
  }

  st.gate_position = gate_position;
  if (this->state_.gate_position != st.gate_position) {
    ESP_LOGD(TAG, "New gate position %u -> %u", this->state_.gate_position, st.gate_position);
  }

  st.flags.sound_state = buzzer;
  if (this->state_.flags.sound_state != st.flags.sound_state) {
    ESP_LOGD(TAG, "New sound state %s -> %s", ONOFF(this->state_.flags.sound_state), ONOFF(st.flags.sound_state));
  }

  st.flags.led_state = led;
  if (this->state_.flags.led_state != st.flags.led_state) {
    ESP_LOGD(TAG, "New led state %s -> %s", ONOFF(this->state_.flags.led_state), ONOFF(st.flags.led_state));
  }

  this->api_->write_state(st, 1);
}

#ifdef TION_ENABLE_PRESETS
bool Tion4s::enable_boost_() {
  if (this->vport_type_ != TionVPortType::VPORT_BLE) {
    return TionClimateComponent::enable_boost_();
  }

  auto boost_time = this->get_boost_time();
  if (boost_time == 0) {
    return false;
  }

  this->api_->set_turbo(boost_time, 1);
  if (this->boost_time_left_) {
    this->boost_time_left_->publish_state(100);
  }

  return true;
}

void Tion4s::cancel_boost_() {
  if (this->vport_type_ != TionVPortType::VPORT_BLE) {
    TionClimateComponent::cancel_boost_();
    return;
  }
  this->api_->set_turbo(0, 1);
}
#endif

}  // namespace tion
}  // namespace esphome
