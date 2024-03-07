#include <cinttypes>
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

void Tion3sClimate::update_state(const tion::TionState &state) {
  if (!state.power_state) {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->action = climate::CLIMATE_ACTION_OFF;
  } else if (state.heater_state) {
    this->mode = climate::CLIMATE_MODE_HEAT;
    this->action = this->mode == state.is_heating(this->api_->traits()) ? climate::CLIMATE_ACTION_HEATING
                                                                        : climate::CLIMATE_ACTION_FAN;
  } else {
    this->mode = climate::CLIMATE_MODE_FAN_ONLY;
    this->action = climate::CLIMATE_ACTION_FAN;
  }

  this->current_temperature = state.current_temperature;
  this->target_temperature = state.target_temperature;
  this->set_fan_speed_(state.fan_speed);
  this->publish_state();
#ifdef USE_TION_VERSION
  if (this->version_ && state.firmware_version > 0) {
    this->version_->publish_state(str_snprintf("%04X", 4, state.firmware_version));
  }
#endif
  if (this->air_intake_) {
    auto air_intake = this->air_intake_->at(static_cast<size_t>(state.gate_position));
    if (air_intake.has_value()) {
      this->air_intake_->publish_state(*air_intake);
    }
  }
#ifdef USE_TION_PRODUCTIVITY
  if (this->productivity_) {
    this->productivity_->publish_state(state.productivity);
  }
#endif
}

void Tion3sClimate::control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, float target_temperature,
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

}  // namespace tion
}  // namespace esphome
