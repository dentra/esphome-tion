#include "esphome/core/defines.h"
#ifdef USE_OTA
#include "esphome/components/ota/ota_component.h"
#endif

#include "esphome/core/log.h"

#include <cstddef>
#include <ctime>
#include <cmath>
#include <cinttypes>

#include "tion_4s_climate.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_4s";

using namespace dentra::tion_4s;

void Tion4sClimate::dump_config() {
  this->dump_settings(TAG, "Tion 4S");
  LOG_SWITCH("  ", "Recirculation", this->recirculation_);
  this->dump_presets(TAG);
}

void Tion4sClimate::setup() {
  Tion4sClimateBase::setup();

#ifdef TION_ENABLE_HEARTBEAT
  if (this->vport_type_ == TionVPortType::VPORT_UART) {
    this->set_interval(this->heartbeat_interval_, [this]() { this->api_->send_heartbeat(); });

#ifdef USE_OTA
    if (this->heartbeat_interval_ > 0) {
      // additionally send heartbeat when OTA starts and before ESP restart.
      ota::global_ota_component->add_on_state_callback([this](ota::OTAState state, float, uint8_t) {
        if (state != ota::OTAState::OTA_IN_PROGRESS) {
          this->api_->send_heartbeat();
        }
      });
    }
#endif
  }
#endif  // TION_ENABLE_HEARTBEAT
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

void Tion4sClimate::update_state(const tion::TionState &state) {
  if (!state.power_state) {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->action = climate::CLIMATE_ACTION_OFF;
  } else if (state.heater_state) {
    this->mode = climate::CLIMATE_MODE_HEAT;
    this->action =
        state.is_heating(this->api_->traits()) ? climate::CLIMATE_ACTION_HEATING : climate::CLIMATE_ACTION_FAN;
  } else {
    this->mode = climate::CLIMATE_MODE_FAN_ONLY;
    this->action = climate::CLIMATE_ACTION_OFF;
  }

  this->current_temperature = state.current_temperature;
  this->target_temperature = state.target_temperature;
  this->set_fan_speed_(state.fan_speed);
  this->publish_state();

  if (this->recirculation_) {
    this->recirculation_->publish_state(state.gate_position != TionGatePosition::OUTDOOR);
  }
}

void Tion4sClimate::control_recirculation_state(bool state) {
  auto *call = this->make_api_call();
  call->set_gate_position(state ? TionGatePosition::INDOOR : TionGatePosition::OUTDOOR);
  call->perform();
}

void Tion4sClimate::control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, float target_temperature,
                                          TionGatePosition gate_position) {
  auto *call = this->make_api_call();

  call->set_fan_speed(fan_speed);
  if (!std::isnan(target_temperature)) {
    call->set_target_temperature(target_temperature);
  }

  if (mode == climate::CLIMATE_MODE_OFF) {
    call->set_power_state(false);
  } else if (mode == climate::CLIMATE_MODE_HEAT_COOL) {
    call->set_power_state(true);
  } else {
    call->set_power_state(true);
    call->set_heater_state(mode == climate::CLIMATE_MODE_HEAT);
  }

  call->set_gate_position(gate_position);

  call->perform();
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

}  // namespace tion
}  // namespace esphome
// #endif  // USE_TION_CLIMATE
