#include <cstdint>
#include <cstring>
#include <cinttypes>

#include "utils.h"
#include "log.h"

#include "tion-api.h"
#include "tion-api-defines.h"

#ifndef TION_DUMP
#define TION_DUMP TION_LOGV
#endif

namespace dentra {
namespace tion {

static const char *const TAG = "tion-api";

bool TionApiBaseWriter::write_frame(uint16_t type, const void *data, size_t size) const {
  TION_LOGV(TAG, "Write frame 0x%04X: %s", type, hexencode(data, size).c_str());
  if (!this->writer_) {
    TION_LOGE(TAG, "Writer is not configured");
    return false;
  }
  return this->writer_(type, data, size);
}

std::string decode_errors(uint32_t errors, uint8_t error_min_bit, uint8_t error_max_bit, uint8_t warning_min_bit,
                          uint8_t warning_max_bit) {
  if (errors == 0) {
    return {};
  }

  std::string error_messages;
  for (uint8_t i = error_min_bit; i <= error_max_bit; i++) {
    uint32_t mask = 1 << i;
    if ((errors & mask) == mask) {
      if (!error_messages.empty()) {
        error_messages += ", ";
      }
      error_messages += "EC";
      error_messages += std::to_string(i + 1);
    }
  }

  if (warning_min_bit != warning_max_bit) {
    for (uint8_t i = warning_min_bit; i <= warning_max_bit; i++) {
      uint32_t mask = 1 << i;
      if ((errors & mask) == mask) {
        if (!error_messages.empty()) {
          error_messages += ", ";
        }
        error_messages += "WS";
        error_messages += std::to_string(i + 1);
      }
    }
  }

  return error_messages;
}

void TionState::dump(const char *TAG, const TionTraits &traits) const {
  if (traits.errors_decoder) {
    const auto errors = traits.errors_decoder(this->errors);
    if (!errors.empty()) {
      TION_LOGW(TAG, "Breezer alert: %s", errors.c_str());
    }
  }

  TION_DUMP(TAG, "power       : %s", ONOFF(this->power_state));
  TION_DUMP(TAG, "heater      : %s", ONOFF(this->heater_state));
  TION_DUMP(TAG, "filter_warn : %s", ONOFF(this->filter_state));
  TION_DUMP(TAG, "fan_speed   : %d", this->fan_speed);
  TION_DUMP(TAG, "target_T    : %d °C", this->target_temperature);
  TION_DUMP(TAG, "outdoor_T   : %d °C", this->outdoor_temperature);
  TION_DUMP(TAG, "current_T   : %d °C", this->current_temperature);
  TION_DUMP(TAG, "gate_pos    : %s", this->get_gate_position_str(traits));

  if (traits.supports_sound_state) {
    TION_DUMP(TAG, "sound       : %s", ONOFF(this->sound_state));
  }
  if (traits.supports_led_state) {
    TION_DUMP(TAG, "led         : %s", ONOFF(this->led_state));
  }

  TION_DUMP(TAG, "auto        : %s", ONOFF(this->auto_state));
  TION_DUMP(TAG, "comm_source : %s", this->comm_source == CommSource::AUTO ? "AUTO" : "USER");

  if (traits.max_heater_power) {
    TION_DUMP(TAG, "heater_max  : %u W", traits.get_max_heater_power());
  }

  if (traits.supports_heater_var) {
    TION_DUMP(TAG, "heater_var  : %u %%", this->heater_var);
  }

  TION_DUMP(TAG, "filter_time : %" PRIu32 " s", this->filter_time_left);
  if (traits.supports_work_time) {
    TION_DUMP(TAG, "work_time   : %" PRIu32 " s", this->work_time);
  }
  if (traits.supports_fan_time) {
    TION_DUMP(TAG, "fan_time    : %" PRIu32 " s", this->fan_time);
  }
  if (traits.supports_airflow_counter) {
    TION_DUMP(TAG, "airflow_cnt : %" PRIu32, this->airflow_counter);
    TION_DUMP(TAG, "airflow_m3  : %.3f m³", this->airflow_m3);
  }

  if (traits.supports_pcb_pwr_temperature) {
    TION_DUMP(TAG, "pcb_pwr_temp: %d °C", this->pcb_pwr_temperature);
  }
  if (traits.supports_pcb_ctl_temperatire) {
    TION_DUMP(TAG, "pcb_ctl_temp: %d °C", this->pcb_ctl_temperature);
  }

  if (this->firmware_version) {
    TION_DUMP(TAG, "firmware_ver: %04X", this->firmware_version);
  }
  if (this->hardware_version) {
    TION_DUMP(TAG, "hardware_ver: %04X", this->hardware_version);
  }

  TION_DUMP(TAG, "errors      : %08" PRIX32, this->errors);
}

float TionState::get_heater_power(const TionTraits &traits) const {
  if (traits.supports_heater_var) {
    return (traits.max_heater_power * this->heater_var) * 0.1f;
  }
  return this->is_heating(traits) ? traits.get_max_heater_power() : 0.0f;
}

bool TionState::is_heating(const TionTraits &traits) const {
  if (traits.supports_heater_var) {
    return this->heater_var > 0;
  }
  if (!this->heater_state || traits.max_heater_power == 0) {
    return false;
  }
  // heating detection borrowed from:
  // https://github.com/TionAPI/tion_python/blob/master/tion_btle/tion.py#L177
  // self.heater_temp - self.in_temp > 3 and self.out_temp > self.in_temp
  return (this->target_temperature - this->outdoor_temperature) > 3 &&
         (this->current_temperature > this->outdoor_temperature);
}

const char *TionState::get_gate_position_str(const TionTraits &traits) const {
  if (traits.supports_gate_error && this->gate_error_state) {
    return "error";
  }
  if (traits.supports_gate_position_change_mixed) {
    switch (this->gate_position) {
      case TionGatePosition::OUTDOOR:
        return "outdoor";
      case TionGatePosition::INDOOR:
        return "indoor";
      case TionGatePosition::MIXED:
        return "mixed";
      default:
        return "unknown";
    }

  } else if (traits.supports_gate_position_change) {
    switch (this->gate_position) {
      case TionGatePosition::OUTDOOR:
        return "inflow";
      case TionGatePosition::INDOOR:
        return "recirculation";
      default:
        return "unknown";
    }
  }
  return this->gate_position == TionGatePosition::OPENED ? "opened" : "closed";
}

TionState TionApiBase::make_write_state_(TionStateCall *call) const {
  const auto &cs = this->state_;
  // new state
  auto ns = this->state_;

  if (call->get_fan_speed().has_value()) {
    const auto fan_speed = *call->get_fan_speed();
    // do not allow to set fan speed to 0
    if (fan_speed == 0) {
      if (call->get_power_state().value_or(cs.power_state)) {
        TION_LOGW(TAG, "Zero fan speed lead to power off");
        call->set_power_state(false);
        call->set_fan_speed(cs.fan_speed);
      }
    } else if (fan_speed > this->traits_.max_fan_speed) {
      TION_LOGW(TAG, "Disallowed fan speed: %u", fan_speed);
      call->set_fan_speed(cs.fan_speed);
    } else {
      if (cs.fan_speed != fan_speed) {
        TION_LOGD(TAG, "New fan speed %u -> %u", cs.fan_speed, fan_speed);
      }
      ns.fan_speed = fan_speed;
    }
  }

  if (call->get_power_state().has_value()) {
    const auto power_state = *call->get_power_state();
    if (cs.power_state != power_state) {
      TION_LOGD(TAG, "New power state %s -> %s", ONOFF(cs.power_state), ONOFF(power_state));
    }
    ns.power_state = power_state;
  }

  if (call->get_heater_state().has_value()) {
    const auto heater_state = *call->get_heater_state();
    if (cs.heater_state != heater_state) {
      TION_LOGD(TAG, "New heater state %s -> %s", ONOFF(cs.heater_state), ONOFF(heater_state));
    }
    ns.heater_state = heater_state;
  }

  if (call->get_target_temperature().has_value()) {
    const auto target_temperature = *call->get_target_temperature();
    if (cs.target_temperature != target_temperature) {
      TION_LOGD(TAG, "New target temperature %d -> %d", cs.target_temperature, target_temperature);
    }
    ns.target_temperature = *call->get_target_temperature();
  }

  if (this->traits_.supports_sound_state) {
    if (call->get_sound_state().has_value()) {
      const auto sound_state = *call->get_sound_state();
      if (cs.sound_state != sound_state) {
        TION_LOGD(TAG, "New sound state %s -> %s", ONOFF(cs.sound_state), ONOFF(sound_state));
      }
      ns.sound_state = *call->get_sound_state();
    }
  }

  if (this->traits_.supports_led_state) {
    if (call->get_led_state().has_value()) {
      const auto led_state = *call->get_led_state();
      if (cs.led_state != led_state) {
        TION_LOGD(TAG, "New led state %s -> %s", ONOFF(cs.led_state), ONOFF(led_state));
      }
      ns.led_state = *call->get_led_state();
    }
  }

  if (this->traits_.supports_gate_position_change) {
    if (call->get_gate_position().has_value()) {
      auto gate_position = *call->get_gate_position();
      switch (gate_position) {
        case TionGatePosition::OUTDOOR: {
          break;
        }
        case TionGatePosition::INDOOR: {
          if (ns.heater_state) {
            TION_LOGW(TAG, "Indoor gate position disallow heater");
            ns.heater_state = false;
          }
        } break;
        case TionGatePosition::MIXED: {
          if (!this->traits_.supports_gate_position_change_mixed) {
            gate_position = cs.gate_position;
          }
          break;
        }
        default:
          gate_position = cs.gate_position;
          break;
      };
      if (cs.gate_position != gate_position) {
        TION_LOGD(TAG, "New gate position %u -> %u", static_cast<uint8_t>(cs.gate_position),
                  static_cast<uint8_t>(gate_position));
      }
      ns.gate_position = gate_position;
    }
  }

  if (this->traits_.supports_antifrize) {
    if (ns.power_state && !ns.heater_state && ns.outdoor_temperature < 0) {
      TION_LOGW(TAG, "Antifrize protection has worked. Heater now enabled.");
      ns.heater_state = true;
    }
  }

  return ns;
}

void TionStateCall::dump() const {
  TION_DUMP(TAG, "TionStateCall:");
  if (this->fan_speed_.has_value()) {
    TION_DUMP(TAG, "  fan     : %u", *this->fan_speed_);
  }
  if (this->target_temperature_.has_value()) {
    TION_DUMP(TAG, "  target T: %d", *this->target_temperature_);
  }
  if (this->gate_position_.has_value()) {
    TION_DUMP(TAG, "  gate pos: %u", static_cast<uint8_t>(*this->gate_position_));
  }
  if (this->power_state_.has_value()) {
    TION_DUMP(TAG, "  power   : %s", ONOFF(*this->power_state_));
  }
  if (this->heater_state_.has_value()) {
    TION_DUMP(TAG, "  heater  : %s", ONOFF(*this->heater_state_));
  }
  if (this->sound_state_.has_value()) {
    TION_DUMP(TAG, "  sound   : %s", ONOFF(*this->sound_state_));
  }
  if (this->led_state_.has_value()) {
    TION_DUMP(TAG, "  led     : %s", ONOFF(*this->led_state_));
  }
  if (this->auto_state_.has_value()) {
    TION_DUMP(TAG, "  auto    : %s", ONOFF(*this->auto_state_));
  }
}

void TionStateCall::perform() { this->api_->write_state(this); }

void TionStateCall::reset() {
  this->fan_speed_.reset();
  this->power_state_.reset();
  this->heater_state_.reset();
  this->target_temperature_.reset();
  this->sound_state_.reset();
  this->led_state_.reset();
  this->gate_position_.reset();
  this->auto_state_.reset();
}

TionApiBase::TionApiBase() { this->traits_.boost_time = TION_BOOST_TIME; }

void TionApiBase::notify_state_(uint32_t request_id) {
  if (this->state_.boost_time_left > 0) {
    if (this->state_.fan_speed != this->traits_.max_fan_speed || !this->state_.power_state) {
      TION_LOGD(TAG, "Boost canceled by user action");
      this->boost_save_state_(this->state_.fan_speed != this->traits_.max_fan_speed);
      this->boost_cancel_(nullptr);
    } else {
      if (!this->traits_.supports_boost) {
        const auto boost_work_time = this->state_.work_time - this->boost_save_.start_time;
        if (boost_work_time < this->traits_.boost_time) {
          this->state_.boost_time_left = this->traits_.boost_time - boost_work_time;
        } else {
          this->boost_cancel_(nullptr);
        }
      }
      TION_DUMP(TAG, "Boost time left %d s", this->state_.boost_time_left);
    }
  }

  if (this->chack_antifrize_(this->state_)) {
    TionStateCall call(this);
    call.set_heater_state(true);
    call.perform();
  }

  this->on_state_fn.call_if(this->state_, request_id);
}

bool TionApiBase::chack_antifrize_(const TionState &cs) const {
  if (this->traits_.supports_antifrize) {
    if (cs.power_state && !cs.heater_state && cs.outdoor_temperature < 0) {
      TION_LOGW(TAG, "Antifrize protection has worked. Heater now enabled.");
      return true;
    }
  }
  return false;
}

void TionApiBase::set_boost_time(uint16_t boost_time) {
  TION_LOGD(TAG, "New boost time: %u s", boost_time);
  this->traits_.boost_time = boost_time;
}

void TionApiBase::enable_boost(bool state, TionStateCall *ext_call) {
  TION_LOGD(TAG, "Switching boost to %s", ONOFF(state));
  if (state) {
    this->boost_enable_(ext_call);
  } else {
    this->boost_cancel_(ext_call);
  }
}

void TionApiBase::boost_enable_(TionStateCall *ext_call) {
  if (!this->state_.is_initialized(this->traits_)) {
    TION_LOGW(TAG, "State is not initialized.");
    return;
  }

  if (this->state_.boost_time_left > 0) {
    TION_LOGW(TAG, "Boost is already in progress, time left %" PRIu32 " s", this->state_.boost_time_left);
    return;
  }

  if (this->state_.fan_speed == this->traits_.max_fan_speed) {
    TION_LOGW(TAG, "Fan is already running at maximum speed");
    return;
  }

  const int boost_time = this->traits_.boost_time;
  if (boost_time == 0) {
    TION_LOGW(TAG, "Boost time is not configured");
    return;
  }

  if (this->traits_.supports_boost) {
    this->enable_native_boost_(true);
    return;
  }

  this->boost_save_state_(true);
  TION_LOGD(TAG, "Schedule boost for %" PRIu32 " s", boost_time);
  this->state_.boost_time_left = boost_time;

  TionStateCall int_call(this);
  TionStateCall *call = ext_call ? ext_call : &int_call;
  call->set_fan_speed(this->traits_.max_fan_speed);
  call->set_power_state(true);
  call->set_gate_position(TionGatePosition::OUTDOOR);
  if (this->traits_.boost_heater_state >= 0) {
    call->set_heater_state(this->traits_.boost_heater_state > 0);
  }
  if (this->traits_.boost_target_temperature != 0) {
    call->set_target_temperature(this->traits_.boost_target_temperature);
  }
  if (ext_call == nullptr) {
    call->perform();
  }
}

void TionApiBase::boost_save_state_(bool save_fan) {
  this->boost_save_.start_time = this->state_.work_time;
  this->boost_save_.power_state = this->state_.power_state;
  this->boost_save_.heater_state = this->state_.heater_state;
  if (save_fan) {
    this->boost_save_.fan_speed = this->state_.fan_speed;
  }
  this->boost_save_.target_temperature = this->state_.target_temperature;
  this->boost_save_.gate_position = this->state_.gate_position;
}

void TionApiBase::boost_cancel_(TionStateCall *ext_call) {
  if (!(this->state_.boost_time_left > 0)) {
    return;
  }
  TION_LOGD(TAG, "Boost finished");

  if (this->traits_.supports_boost) {
    this->enable_native_boost_(false);
    return;
  }

  this->state_.boost_time_left = 0;
  this->enable_preset_(this->boost_save_, ext_call);
}

void TionApiBase::enable_preset_(const PresetData &preset, TionStateCall *ext_call) {
  TionStateCall int_call(this);
  TionStateCall *call = ext_call ? ext_call : &int_call;
  if (preset.power_state >= 0) {
    call->set_power_state(preset.power_state > 0);
  }
  if (preset.heater_state >= 0) {
    call->set_heater_state(preset.heater_state > 0);
  }
  if (preset.fan_speed != 0) {
    call->set_fan_speed(preset.fan_speed);
  }
  if (preset.target_temperature != 0) {
    call->set_target_temperature(preset.target_temperature);
  }
  if (preset.gate_position != TionGatePosition::UNKNOWN) {
    call->set_gate_position(preset.gate_position);
  }
  if (ext_call == nullptr) {
    call->perform();
  }
}

void TionApiBase::enable_preset(const std::string &preset, TionStateCall *call) {
  TION_LOGD(TAG, "Activate preset '%s'", preset.c_str());
  if (preset.empty() || strcasecmp(preset.c_str(), "none") == 0) {
    this->active_preset_ = preset;
    return;
  }
  const auto &it = this->presets_.find(preset);
  if (it == this->presets_.end()) {
    TION_LOGD(TAG, "Preset '%s' not found", preset.c_str());
    return;
  }
  this->active_preset_ = preset;
  this->enable_preset_(it->second, call);
}

std::vector<std::string> TionApiBase::get_presets() const {
  std::vector<std::string> presets;
  presets.push_back("none");
  for (auto &&preset : this->presets_) {
    presets.push_back(preset.first);
  }
  return presets;
};

void TionApiBase::add_preset(const std::string &name, const PresetData &data) {
  if (name.empty()) {
    TION_LOGW(TAG, "Empty preset name");
    return;
  }
  if (strcasecmp(name.c_str(), "none") == 0) {
    TION_LOGW(TAG, "Skip reserved preset 'none'");
    return;
  }
  if (data.target_temperature == 0 && data.heater_state < 0 && data.power_state < 0 && data.fan_speed == 0 &&
      data.gate_position == TionGatePosition::UNKNOWN) {
    TION_LOGW(TAG, "Preset '%s' has no data to change", name.c_str());
    return;
  }
  if (data.target_temperature < this->traits_.min_target_temperature ||
      data.target_temperature > this->traits_.max_target_temperature) {
    TION_LOGW(TAG, "Preset '%s' has invalid target temperature %d", name.c_str(), data.target_temperature);
    return;
  }
  if (data.fan_speed > this->traits_.max_fan_speed) {
    TION_LOGW(TAG, "Preset '%s' has invalid fan speed %u", name.c_str(), data.fan_speed);
    return;
  }
  TION_LOGD(TAG, "Setup preset '%s': %d, %d, %u, %d, %u", name.c_str(), data.power_state, data.heater_state,
            data.fan_speed, data.target_temperature, static_cast<uint8_t>(data.gate_position));
  this->presets_.emplace(name, data);
}

}  // namespace tion
}  // namespace dentra
