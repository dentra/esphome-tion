#include <cinttypes>

#include "esphome/core/log.h"
#include "tion_lt_climate.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_lt_climate";

void TionLtClimate::dump_config() {
  this->dump_settings(TAG, "Tion Lite");
  LOG_BINARY_SENSOR("  ", "Gate State", this->gate_state_);
  this->dump_presets(TAG);
}

void TionLtClimate::update_state(const TionState &state) {
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

  if (this->gate_state_) {
    this->gate_state_->publish_state(state.get_gate_state());
  }
}

void TionLtClimate::control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, float target_temperature,
                                          TionGatePosition /*gate_position*/) {
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

  call->perform();
}

}  // namespace tion
}  // namespace esphome
