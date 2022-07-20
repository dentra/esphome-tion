#include <inttypes.h>

#include "esphome/core/log.h"

#include "tion_4s.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_4s";

void Tion4s::on_ready() { this->request_dev_status(); }

void Tion4s::read(const tion_dev_status_t &status) {
  if (status.device_type != tion_dev_status_t::BR4S) {
    this->parent_->set_enabled(false);
    ESP_LOGE(TAG, "Unsupported device type %04X", status.device_type);
    return;
  }
  this->read_dev_status_(status);
  // this->request_errors();
  // this->request_test();
  this->request_time();
  // this->request_timers();
  this->run_polling();
}

void Tion4s::run_polling() {
  this->request_turbo();
  this->request_state();
  this->schedule_disconnect(this->state_timeout_);
}

void Tion4s::read(const tion4s_time_t &tion_time) {
  time_t time = tion_time.get_time();
  auto c_tm = ::gmtime(&time);
  char buf[20] = {};
  strftime(buf, sizeof(buf), "%F %T", c_tm);
  ESP_LOGD(TAG, "Device time %s", buf);
}

void Tion4s::read(const tion4s_turbo_t &turbo) {
  if (this->is_dirty_(DIRTY_BOOST_ENABLE)) {
    this->drop_dirty_(DIRTY_BOOST_ENABLE);
    uint16_t turbo_time = this->boost_time_ ? this->boost_time_->state * 60 : DEFAULT_BOOST_TIME_SEC;
    if (turbo_time > 0) {
      this->set_turbo_time(turbo_time);
      if (this->boost_time_left_) {
        this->boost_time_left_->publish_state(100);
      }
    } else {
      // fallback on error
      this->enable_preset_(this->saved_preset_);
    }
    return;
  }

  if (this->is_dirty_(DIRTY_BOOST_CANCEL)) {
    this->drop_dirty_(DIRTY_BOOST_CANCEL);
    if (turbo.turbo_time > 0) {
      this->set_turbo_time(0);
    }
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
      uint16_t boost_time = this->boost_time_ ? this->boost_time_->state * 60 : DEFAULT_BOOST_TIME_SEC;
      this->boost_time_left_->publish_state(static_cast<float>(turbo.turbo_time) /
                                            static_cast<float>(boost_time / 100));
    }
  }

  this->publish_state();
}

void Tion4s::read(const tion4s_state_t &state) {
  if (this->is_dirty_(DIRTY_STATE)) {
    this->drop_dirty_(DIRTY_STATE);
    this->flush_state_(state);
    return;
  }

  this->cancel_disconnect();

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

  ESP_LOGV(TAG, "sound_state    : %s", ONOFF(state.flags.sound_state));
  ESP_LOGV(TAG, "led_state      : %s", ONOFF(state.flags.led_state));
  ESP_LOGV(TAG, "current_temp   : %d", state.current_temperature);
  ESP_LOGV(TAG, "outdoor_temp   : %d", state.outdoor_temperature);
  ESP_LOGV(TAG, "heater_power   : %f", state.heater_power());
  ESP_LOGV(TAG, "airflow_counter: %f", state.counters.airflow_counter());
  ESP_LOGV(TAG, "filter_warnout : %s", ONOFF(state.flags.filter_warnout));
  ESP_LOGV(TAG, "filter_time    : %u", state.counters.filter_time);
  ESP_LOGV(TAG, "gate_position       : %u", state.gate_position);

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

  if (!this->is_persistent_connection()) {
    this->schedule_disconnect();
  }
}

void Tion4s::flush_state_(const tion4s_state_t &state_) const {
  tion4s_state_t state = state_;

  auto power_state = this->mode != climate::CLIMATE_MODE_OFF;
  if (state.flags.power_state != power_state) {
    ESP_LOGD(TAG, "New power state %s", ONOFF(power_state));
    state.flags.power_state = power_state;
  }

  auto heater_mode = this->mode == climate::CLIMATE_MODE_HEAT ? tion4s_state_t::HEATER_MODE_HEATING
                                                              : tion4s_state_t::HEATER_MODE_TEMPERATURE_MAINTENANCE;
  if (state.flags.heater_mode != heater_mode) {
    ESP_LOGD(TAG, "New heater mode %s", ONOFF(heater_mode));
    state.flags.heater_mode = heater_mode;
  }

  if (this->recirculation_) {
    auto gate_position = this->recirculation_->state ? tion4s_state_t::GATE_POSITION_RECIRCULATION
                                                     : tion4s_state_t::GATE_POSITION_INFLOW;
    if (state.gate_position != gate_position) {
      ESP_LOGD(TAG, "New gate position %u", gate_position);
      state.gate_position = gate_position;
    }
  }

  int8_t target_temperature = this->target_temperature;
  if (state.target_temperature != target_temperature) {
    ESP_LOGD(TAG, "New target temperature %d", target_temperature);
    state.target_temperature = target_temperature;
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

  if (this->custom_fan_mode.has_value()) {
    auto fan_speed = this->get_fan_speed_();
    if (state.fan_speed != fan_speed) {
      ESP_LOGD(TAG, "New fan speed %u", fan_speed);
      state.fan_speed = fan_speed;
    }
  }

  TionApi4s::write_state(state);
}

bool Tion4s::enable_boost_() {
  // TODO update turbo state
  this->set_dirty_(DIRTY_BOOST_ENABLE);
  return true;
}

void Tion4s::cancel_boost_() {
  // TODO update turbo state
  this->set_dirty_(DIRTY_BOOST_CANCEL);
}

bool Tion4s::write_state() {
  this->publish_state();
  this->set_dirty_(DIRTY_STATE);
  if (this->is_persistent_connection() && this->is_connected()) {
    this->request_state();
  } else {
    this->parent_->set_enabled(true);
  }
  return true;
}

void Tion4s::update() {
  if (this->is_persistent_connection() && this->is_connected()) {
    this->run_polling();
  } else {
    this->parent_->set_enabled(true);
  }
}

}  // namespace tion
}  // namespace esphome
