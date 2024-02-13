#include <cinttypes>
#include "esphome/core/log.h"
#include "esphome/core/defines.h"

#include "tion_o2_climate.h"

#ifdef TION_ENABLE_ANTIFRIZE
#define TION_OPTION_STR_ANTIFRIZE "enabled"
#else
#define TION_OPTION_STR_ANTIFRIZE "disabled"
#endif

namespace esphome {
namespace tion {

static const char *const TAG = "tion_o2_climate";

void TionO2Climate::dump_config() {
  this->dump_settings(TAG, "Tion O2");
  ESP_LOGCONFIG("  ", "Antifrize: %s", TION_OPTION_STR_ANTIFRIZE);
  this->dump_presets(TAG);
}

void TionO2Climate::update_state(const tiono2_state_t &state) {
  this->dump_state(state);

  if (!state.power_state) {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->action = climate::CLIMATE_ACTION_OFF;
  } else if (state.heater_state) {
    this->mode = climate::CLIMATE_MODE_HEAT;
    this->action = this->mode == state.is_heating() ? climate::CLIMATE_ACTION_HEATING : climate::CLIMATE_ACTION_FAN;
  } else {
    this->mode = climate::CLIMATE_MODE_FAN_ONLY;
    this->action = climate::CLIMATE_ACTION_FAN;
  }

  this->current_temperature = state.current_temperature;
  this->target_temperature = state.target_temperature;
  this->set_fan_speed_(state.fan_speed);
  this->publish_state();
#ifdef USE_TION_PRODUCTIVITY
  if (this->productivity_) {
    this->productivity_->publish_state(state.productivity);
  }
#endif
}

void TionO2Climate::dump_state(const tiono2_state_t &state) const {
  static char flags_bits[CHAR_BIT + 1]{};
  for (int i = 0; i < CHAR_BIT; i++) {
    flags_bits[7 - i] = ((state.flags >> i) & 1) + '0';
  }
  ESP_LOGD(TAG, "flags       : %s", flags_bits);
  ESP_LOGV(TAG, "filter_state: %s", ONOFF(state.filter_state));
  ESP_LOGV(TAG, "power_state : %s", ONOFF(state.power_state));
  ESP_LOGV(TAG, "heater_state: %s", ONOFF(state.heater_state));
  ESP_LOGV(TAG, "outdoor_temp: %d", state.outdoor_temperature);
  ESP_LOGV(TAG, "current_temp: %d", state.current_temperature);
  ESP_LOGV(TAG, "target_temp : %d", state.current_temperature);
  ESP_LOGV(TAG, "fan_speed   : %d", state.fan_speed);
  ESP_LOGV(TAG, "productivity: %d", state.productivity);
  ESP_LOGV(TAG, "unknown7    : %d", state.unknown7);
  ESP_LOGV(TAG, "errors      : %d", state.errors);
  ESP_LOGV(TAG, "work_time   : %" PRIu32, state.counters.work_time_days());
  ESP_LOGV(TAG, "filter_time : %" PRIu32, state.counters.filter_time_left());
  if (state.unknown7 != 0x04) {
    ESP_LOGW(TAG, "Please report unknown7=%02X", state.unknown7);
  }
  state.for_each_error([](auto code, auto type) { ESP_LOGW(TAG, "Breezer alert: %s%02u", type, code); });
}

void TionO2Climate::control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, float target_temperature,
                                          TionGatePosition gate_position) {
  auto call = this->make_tion_call();
  call.set_fan_speed(fan_speed);

  if (!std::isnan(target_temperature)) {
    call.set_target_temperature(target_temperature);
  }

  if (mode == climate::CLIMATE_MODE_OFF) {
    call.set_power_state(false);
  } else if (mode == climate::CLIMATE_MODE_HEAT_COOL) {
    call.set_power_state(true);
  } else {
    call.set_power_state(true);
    call.set_heater_state(mode == climate::CLIMATE_MODE_HEAT);
  }

  call.perform();
}

void TionO2Climate::TionO2Call::perform() {
  // cur state
  const auto &cs = this->parent_->state_;
  // new state
  auto ns = cs;

  if (this->power_state_.has_value()) {
    ns.power_state = *this->power_state_;
    if (cs.power_state != ns.power_state) {
      ESP_LOGD(TAG, "New power state %s -> %s", ONOFF(cs.power_state), ONOFF(ns.power_state));
    }
  }

  if (this->heater_state_.has_value()) {
    ns.heater_state = *this->heater_state_;
    if (cs.heater_state != ns.heater_state) {
      ESP_LOGD(TAG, "New heater state %s -> %s", ONOFF(cs.heater_state), ONOFF(ns.heater_state));
    }
  }

  if (this->fan_speed_.has_value()) {
    ns.fan_speed = *this->fan_speed_;
    if (cs.fan_speed != ns.fan_speed) {
      ESP_LOGD(TAG, "New fan speed %u -> %u", cs.fan_speed, ns.fan_speed);
    }
  }

  if (this->target_temperature_.has_value()) {
    ns.target_temperature = *this->target_temperature_;
    if (cs.target_temperature != ns.target_temperature) {
      ESP_LOGD(TAG, "New target temperature %d -> %d", cs.target_temperature, ns.target_temperature);
    }
  }

#ifdef TION_ENABLE_ANTIFRIZE
  if (ns.power_state && !ns.heater_state && ns.outdoor_temperature < 0) {
    ESP_LOGW(TAG, "Antifrize protection has worked. Heater now enabled.");
    ns.heater_state = true;
  }
#endif

  this->parent_->write_api_state_(ns);
}

}  // namespace tion
}  // namespace esphome
