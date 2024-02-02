#include "esphome/core/log.h"

#include <cstddef>
#include <ctime>
#include <cmath>
#include <cinttypes>

#include "tion_4s_climate.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_4s";

void Tion4sClimate::dump_config() {
  this->dump_settings(TAG, "Tion 4S");
  LOG_SWITCH("  ", "Recirculation", this->recirculation_);
  this->dump_presets(TAG);
}

#ifdef TION_ENABLE_PRESETS
void Tion4sClimate::on_turbo(const tion4s_turbo_t &turbo, uint32_t request_id) {
  if (this->vport_type_ != TionVPortType::VPORT_BLE) {
    ESP_LOGW(TAG, "Only BLE supports native turbo mode. Please report.");
    return;
  }

  // change preset if turbo changed outside
  const auto preset = this->preset.value_or(climate::CLIMATE_PRESET_NONE);
  if (turbo.is_active) {
    if (preset != climate::CLIMATE_PRESET_BOOST) {
      this->presets_saved_preset_ = preset;
      this->preset = climate::CLIMATE_PRESET_BOOST;
    }
  } else {
    if (preset == climate::CLIMATE_PRESET_BOOST) {
      this->presets_cancel_boost_(this);
    }
  }

  if (this->presets_boost_time_left_) {
    if (turbo.turbo_time == 0) {
      this->presets_boost_time_left_->publish_state(NAN);
    } else {
      const auto boost_time = this->get_boost_time_();
      const auto boost_time_one_percent = boost_time / 100;
      this->presets_boost_time_left_->publish_state(static_cast<float>(turbo.turbo_time) /
                                                    static_cast<float>(boost_time_one_percent));
    }
  }

  this->publish_state();
}
#endif

void Tion4sClimate::update_state(const tion4s_state_t &state) {
  this->dump_state(state);

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

  if (this->recirculation_) {
    this->recirculation_->publish_state(state.gate_position != tion4s_state_t::GATE_POSITION_INFLOW);
  }

#ifdef USE_TION_ERRORS
  if (this->errors_) {
    std::string codes;
    this->enum_errors(state.errors, [&codes](auto code) { codes += (codes.empty() ? "" : ", ") + code; });
    this->errors_->publish_state(codes);
  }
#endif
}

void Tion4sClimate::dump_state(const tion4s_state_t &state) const {
  ESP_LOGV(TAG, "power_state    : %s", ONOFF(state.flags.power_state));
  ESP_LOGV(TAG, "gate_position  : %u", state.gate_position);
  ESP_LOGV(TAG, "fan_speed      : %u", state.fan_speed);
  ESP_LOGV(TAG, "current_temp   : %d", state.current_temperature);
  ESP_LOGV(TAG, "outdoor_temp   : %d", state.outdoor_temperature);
  ESP_LOGV(TAG, "heater_mode    : %u", state.flags.heater_mode);
  ESP_LOGV(TAG, "heater_state   : %s", ONOFF(state.flags.heater_state));
  ESP_LOGV(TAG, "heater_present : %u", state.flags.heater_present);
  ESP_LOGV(TAG, "heater_var     : %u", state.heater_var);

  ESP_LOGV(TAG, "sound_state    : %s", ONOFF(state.flags.sound_state));
  ESP_LOGV(TAG, "led_state      : %s", ONOFF(state.flags.led_state));
  ESP_LOGV(TAG, "filter_warnout : %s", ONOFF(state.flags.filter_warnout));
  ESP_LOGV(TAG, "filter_time    : %" PRIu32, state.counters.filter_time);

  ESP_LOGV(TAG, "airflow_counter: %" PRIu32, state.counters.airflow_counter);
  ESP_LOGV(TAG, "fan_time       : %" PRIu32, state.counters.fan_time);
  ESP_LOGV(TAG, "work_time      : %" PRIu32, state.counters.work_time);

  ESP_LOGV(TAG, "active_timer   : %s", ONOFF(state.flags.active_timer));
  ESP_LOGV(TAG, "pcb_pwr_temp   : %d", state.pcb_pwr_temperature);
  ESP_LOGV(TAG, "pcb_ctl_temp   : %d", state.pcb_ctl_temperature);

  ESP_LOGV(TAG, "ma_connect     : %s", ONOFF(state.flags.ma_connect));
  ESP_LOGV(TAG, "ma_auto        : %s", ONOFF(state.flags.ma_auto));
  ESP_LOGV(TAG, "last_com_source: %u", state.flags.last_com_source);
  ESP_LOGV(TAG, "errors         : %08" PRIX32, state.errors);
  ESP_LOGV(TAG, "reserved       : %02X", state.flags.reserved);

  this->enum_errors(state.errors, [this](auto code) { ESP_LOGW(TAG, "Breezer alert: %s", code.c_str()); });
}

void Tion4sClimate::control_recirculation_state(bool state) {
  ControlState control{};
  control.gate_position = state ? tion4s_state_t::GATE_POSITION_RECIRCULATION : tion4s_state_t::GATE_POSITION_INFLOW;

  if (control.gate_position == tion4s_state_t::GATE_POSITION_RECIRCULATION) {
    // TODO необходимо проверять batch_active
    if (this->mode == climate::CLIMATE_MODE_HEAT) {
      ESP_LOGW(TAG, "Enabled recirculation allow only FAN_ONLY mode");
      control.heater_mode = tion4s_state_t::HeaterMode::HEATER_MODE_TEMPERATURE_MAINTENANCE;
    }
  }

  this->control_state_(control);
}

void Tion4sClimate::control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, float target_temperature,
                                          TionGatePosition gate_position) {
  ControlState control{};

  control.fan_speed = fan_speed;
  if (!std::isnan(target_temperature)) {
    control.target_temperature = target_temperature;
  }

  if (mode == climate::CLIMATE_MODE_OFF) {
    control.power_state = false;
  } else if (mode == climate::CLIMATE_MODE_HEAT_COOL) {
    control.power_state = true;
  } else {
    control.power_state = true;
    control.heater_mode = mode == climate::CLIMATE_MODE_HEAT ? tion4s_state_t::HEATER_MODE_HEATING
                                                             : tion4s_state_t::HEATER_MODE_TEMPERATURE_MAINTENANCE;
  }

  switch (gate_position) {
    case TionGatePosition::OUTDOOR:
      control.gate_position = tion4s_state_t::GATE_POSITION_INFLOW;
      break;
    case TionGatePosition::INDOOR:
      control.gate_position = tion4s_state_t::GATE_POSITION_RECIRCULATION;
      break;
    default:
      control.gate_position = this->get_gate_position_();
      break;
  }

  if (mode == climate::CLIMATE_MODE_HEAT) {
    if (control.gate_position == tion4s_state_t::GATE_POSITION_RECIRCULATION) {
      ESP_LOGW(TAG, "HEAT mode allow only disabled recirculation");
      control.gate_position = tion4s_state_t::GATE_POSITION_INFLOW;
    }
  }

  this->control_state_(control);
}

void Tion4sClimate::control_state_(const ControlState &state) {
  tion4s_state_t st = this->state_;

  if (state.power_state.has_value()) {
    st.flags.power_state = *state.power_state;
    if (this->state_.flags.power_state != st.flags.power_state) {
      ESP_LOGD(TAG, "New power state %s -> %s", ONOFF(this->state_.flags.power_state), ONOFF(st.flags.power_state));
    }
  }

  if (state.heater_mode.has_value()) {
    st.flags.heater_mode = *state.heater_mode;
    if (this->state_.flags.heater_mode != st.flags.heater_mode) {
      ESP_LOGD(TAG, "New heater mode %s -> %s", ONOFF(this->state_.flags.heater_mode), ONOFF(st.flags.heater_mode));
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

  if (state.gate_position.has_value()) {
    st.gate_position = *state.gate_position;
    if (this->state_.gate_position != st.gate_position) {
      ESP_LOGD(TAG, "New gate position %u -> %u", this->state_.gate_position, st.gate_position);
    }
  }

  if (state.buzzer.has_value()) {
    st.flags.sound_state = *state.buzzer;
    if (this->state_.flags.sound_state != st.flags.sound_state) {
      ESP_LOGD(TAG, "New sound state %s -> %s", ONOFF(this->state_.flags.sound_state), ONOFF(st.flags.sound_state));
    }
  }

  if (state.led.has_value()) {
    st.flags.led_state = *state.led;
    if (this->state_.flags.led_state != st.flags.led_state) {
      ESP_LOGD(TAG, "New led state %s -> %s", ONOFF(this->state_.flags.led_state), ONOFF(st.flags.led_state));
    }
  }

  this->write_api_state_(st);
}

#ifdef TION_ENABLE_PRESETS
bool Tion4sClimate::enable_boost() {
  if (this->vport_type_ != TionVPortType::VPORT_BLE) {
    return TionClimateComponent::enable_boost();
  }

  auto boost_time = this->get_boost_time_();
  if (boost_time == 0) {
    return false;
  }

  this->api_->set_turbo(boost_time, ++this->request_id_);
  if (this->presets_boost_time_left_) {
    this->presets_boost_time_left_->publish_state(100);
  }

  return true;
}

void Tion4sClimate::cancel_boost() {
  if (this->vport_type_ != TionVPortType::VPORT_BLE) {
    TionClimateComponent::cancel_boost();
    return;
  }
  this->api_->set_turbo(0, ++this->request_id_);
}
#endif

#ifdef TION_ENABLE_SCHEDULER

void Tion4sClimate::on_time(time_t time, uint32_t request_id) {
  auto *c_tm = std::gmtime(&time);
  char buf[20] = {};
  std::strftime(buf, sizeof(buf), "%F %T", c_tm);
  ESP_LOGI(TAG, "Device UTC time: %s", buf);
  c_tm = std::localtime(&time);
  std::strftime(buf, sizeof(buf), "%F %T", c_tm);
  ESP_LOGI(TAG, "Device local time: %s", buf);
}

namespace {
void add_timer_week_day(std::string &s, bool day, const char *day_name) {
  if (day) {
    if (!s.empty()) {
      s.append(", ");
    }
    s.append(day_name);
  }
}
}  // namespace

void Tion4sClimate::on_timer(uint8_t timer_id, const tion4s_timer_t &timer, uint32_t request_id) {
  std::string schedule;
  if (timer.schedule.monday && timer.schedule.tuesday && timer.schedule.wednesday && timer.schedule.thursday &&
      timer.schedule.friday && timer.schedule.saturday && timer.schedule.sunday) {
    schedule = "MON-SUN";
  } else {
    add_timer_week_day(schedule, timer.schedule.monday, "MON");
    add_timer_week_day(schedule, timer.schedule.tuesday, "TUE");
    add_timer_week_day(schedule, timer.schedule.wednesday, "WED");
    add_timer_week_day(schedule, timer.schedule.thursday, "THU");
    add_timer_week_day(schedule, timer.schedule.friday, "FRI");
    add_timer_week_day(schedule, timer.schedule.saturday, "SAT");
    add_timer_week_day(schedule, timer.schedule.sunday, "SUN");
  }

  ESP_LOGI(TAG, "Timer[%u] %s at %02u:%02u is %s", timer_id, schedule.c_str(), timer.schedule.hours,
           timer.schedule.minutes, ONOFF(timer.timer_state));
}

void Tion4sClimate::on_timers_state(const tion4s_timers_state_t &timers_state, uint32_t request_id) {
  for (int i = 0; i < tion4s_timers_state_t::TIMERS_COUNT; i++) {
    ESP_LOGI(TAG, "Timer[%d] state %s", i, ONOFF(timers_state.timers[i].active));
  }
}

void Tion4sClimate::dump_timers() {
  this->api_->request_time(++this->request_id_);
  for (uint8_t timer_id = 0; timer_id < tion4s_timers_state_t::TIMERS_COUNT; timer_id++) {
    this->api_->request_timer(timer_id, ++this->request_id_);
  }
  this->api_->request_timers_state(++this->request_id_);
}

void Tion4sClimate::reset_timers() {
  const tion4s_timer_t timer{};
  for (uint8_t timer_id = 0; timer_id < tion4s_timers_state_t::TIMERS_COUNT; timer_id++) {
    this->api_->write_timer(timer_id, timer, ++this->request_id_);
  }
}
#endif

void Tion4sClimate::enum_errors(uint32_t errors, const std::function<void(const std::string &)> &fn) const {
  if (errors == 0) {
    return;
  }
  for (uint32_t i = tion4s_state_t::ERROR_MIN_BIT; i <= tion4s_state_t::ERROR_MAX_BIT; i++) {
    uint32_t mask = 1 << i;
    if ((errors & mask) == mask) {
      fn(str_snprintf("EC%02" PRIu32, 4, i + 1));
    }
  }
  for (uint32_t i = tion4s_state_t::WARNING_MIN_BIT; i <= tion4s_state_t::WARNING_MAX_BIT; i++) {
    uint32_t mask = 1 << i;
    if ((errors & mask) == mask) {
      fn(str_snprintf("WS%02" PRIu32, 4, i + 1));
    }
  }
}

}  // namespace tion
}  // namespace esphome
