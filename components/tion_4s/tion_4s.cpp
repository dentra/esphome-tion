#include <inttypes.h>

#include "esphome/core/log.h"
#include "esphome/core/application.h"

#include "tion_4s.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_4s";

climate::ClimateTraits Tion4s::traits() {
  auto traits = TionClimate::traits();
  traits.set_supported_presets({climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_BOOST});
  return traits;
}

void Tion4s::control(const climate::ClimateCall &call) {
  if (call.get_preset().has_value()) {
    // TODO update turbo state
    this->update_flag_ |= UPDATE_BOOST;
  }
  TionClimate::control(call);
}

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
  this->request_turbo();
  // this->request_timers();

  this->request_state();
};

void Tion4s::read(const tion4s_time_t &time) {
  auto tm = time::ESPTime::from_epoch_utc(time.unix_time);
  ESP_LOGD(TAG, "Device time %s", tm.strftime("%F %T").c_str());
}

void Tion4s::read(const tion4s_turbo_t &turbo) {
  if ((this->update_flag_ & UPDATE_BOOST) != 0) {
    this->update_flag_ &= ~UPDATE_BOOST;
    uint16_t turbo_time = this->boost_time_ ? this->boost_time_->state : turbo.turbo_time;
    if (turbo_time > 0) {
      TionApi4s::set_turbo_time(turbo_time);
    }
    return;
  }
  this->preset = turbo.is_active ? climate::CLIMATE_PRESET_BOOST : climate::CLIMATE_PRESET_NONE;
  if (this->boost_time_) {
    this->boost_time_->publish_state(turbo.turbo_time);
  }
  this->publish_state();
}

void Tion4s::read(const tion4s_state_t &state) {
  if ((this->update_flag_ & UPDATE_STATE) != 0) {
    this->update_flag_ &= ~UPDATE_STATE;
    tion4s_state_t st = state;
    this->update_state_(st);
    TionApi4s::write_state(st);
    return;
  }

  this->max_fan_speed_ = state.limits.max_fan_speed;

  if (state.system.power_state) {
    this->mode = state.system.heater_mode == tion4s_state_t::HEATER_MODE_HEATING ? climate::CLIMATE_MODE_HEAT
                                                                                 : climate::CLIMATE_MODE_FAN_ONLY;
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
  }

  this->target_temperature = state.system.target_temperature;

  this->set_fan_mode_(state.system.fan_speed);

  if (this->buzzer_) {
    this->buzzer_->publish_state(state.system.sound_state);
  } else {
    ESP_LOGV(TAG, "sound_state                : %s", ONOFF(state.system.sound_state));
  }
  if (this->led_) {
    this->led_->publish_state(state.system.led_state);
  } else {
    ESP_LOGV(TAG, "led_state                  : %s", ONOFF(state.system.led_state));
  }
  if (this->temp_in_) {
    this->temp_in_->publish_state(state.sensors.indoor_temperature);
  } else {
    ESP_LOGV(TAG, "indoor_temp    : %d", state.sensors.indoor_temperature);
  }
  if (this->temp_out_) {
    this->temp_out_->publish_state(state.sensors.outdoor_temperature);
  } else {
    ESP_LOGV(TAG, "outdoor_temp   : %d", state.sensors.outdoor_temperature);
  }
  if (this->heater_power_) {
    this->heater_power_->publish_state(state.heater_power());
  } else {
    ESP_LOGV(TAG, "heater_power   : %f", state.heater_power());
  }
  if (this->airflow_counter_) {
    this->airflow_counter_->publish_state(state.counters.airflow_counter());
  } else {
    ESP_LOGV(TAG, "airflow_counter: %f", state.counters.airflow_counter());
  }
  if (this->filter_warnout_) {
    this->filter_warnout_->publish_state(state.system.filter_wornout);
  } else {
    ESP_LOGV(TAG, "filter_wornout : %s", ONOFF(state.system.filter_wornout));
  }
  if (this->filter_days_left_) {
    this->filter_days_left_->publish_state(state.counters.fileter_days());
  } else {
    ESP_LOGV(TAG, "filter_time    : %u", state.counters.filter_time);
  }
  if (this->recirculation_) {
    this->recirculation_->publish_state(state.system.substate == tion4s_state_t::SUBSTATE_RECIRCULATION);
  } else {
    ESP_LOGV(TAG, "substate       : %u", state.system.substate);
  }

  ESP_LOGV(TAG, "pcb_pwr_temp   : %d", state.sensors.pcb_pwr_temperature);
  ESP_LOGV(TAG, "pcb_ctl_temp   : %d", state.sensors.pcb_ctl_temperature);
  ESP_LOGV(TAG, "fan_speed      : %u", state.system.fan_speed);
  ESP_LOGV(TAG, "heater_mode    : %u", state.system.heater_mode);
  ESP_LOGV(TAG, "heater_state   : %s", ONOFF(state.system.heater_state));
  ESP_LOGV(TAG, "heater_present : %u", state.system.heater_present);
  ESP_LOGV(TAG, "heater_var     : %u", state.heater_var);
  ESP_LOGV(TAG, "last_com_source: %u", state.system.last_com_source);

  ESP_LOGV(TAG, "ma             : %s", ONOFF(state.system.ma));
  ESP_LOGV(TAG, "ma_auto        : %s", ONOFF(state.system.ma_auto));
  ESP_LOGV(TAG, "active_timer   : %s", ONOFF(state.system.active_timer));
  ESP_LOGV(TAG, "reserved       : %02x", state.system.reserved);
  ESP_LOGV(TAG, "work_time      : %u", state.counters.work_time);
  ESP_LOGV(TAG, "fan_time       : %u", state.counters.fan_time);
  ESP_LOGV(TAG, "errors         : %u", state.errors);

  this->publish_state();

  // leave 3 sec connection left for end all of jobs
  App.scheduler.set_timeout(this, TAG, 3000, [this]() { this->parent_->set_enabled(false); });
}

void Tion4s::update_state_(tion4s_state_t &state) const {
  state.system.power_state = this->mode != climate::CLIMATE_MODE_OFF;
  state.system.heater_mode = this->mode == climate::CLIMATE_MODE_HEAT
                                 ? tion4s_state_t::HEATER_MODE_HEATING
                                 : tion4s_state_t::HEATER_MODE_TEMPERATURE_MAINTENANCE;
  if (this->recirculation_) {
    state.system.substate =
        this->recirculation_->state ? tion4s_state_t::SUBSTATE_RECIRCULATION : tion4s_state_t::SUBSTATE_INFLOW;
  }

  state.system.target_temperature = this->target_temperature;

  if (this->led_) {
    state.system.led_state = this->led_->state;
  }

  if (this->buzzer_) {
    state.system.sound_state = this->buzzer_->state;
  }

  if (this->custom_fan_mode.has_value()) {
    state.system.fan_speed = this->get_fan_speed();
  }
}

}  // namespace tion
}  // namespace esphome