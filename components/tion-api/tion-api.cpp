#include <cstdint>
#include <cstring>
#include <cinttypes>

#include "utils.h"
#include "log.h"

#include "tion-api.h"
#include "tion-api-defines.h"

namespace dentra {
namespace tion {

static const char *const TAG = "tion-api";
#define INVALID_STATE_CALL() TION_LOGW(TAG, "Invalid state call")

void report_errors(uint32_t errors, uint8_t error_min_bit, uint8_t error_max_bit, uint8_t warning_min_bit,
                   uint8_t warning_max_bit) {}

void enum_errors(uint32_t errors, uint8_t min_bit, uint8_t max_bit, const void *param,
                 const std::function<void(uint8_t, const void *)> &fn) {
  for (uint8_t i = min_bit; i <= max_bit; i++) {
    uint32_t mask = 1 << i;
    if ((errors & mask) == mask) {
      fn(i - min_bit + 1, param);
    }
  }
}

std::string decode_errors(uint32_t errors, uint8_t error_min_bit, uint8_t error_max_bit, uint8_t warning_min_bit,
                          uint8_t warning_max_bit) {
  std::string messages;

  auto add_message = [&messages](uint8_t err, const void *param) {
    if (!messages.empty()) {
      messages += ", ";
    }
    messages += static_cast<const char *>(param);
    if (err < 10) {
      messages += '0';
    }
    messages += std::to_string(err);
  };

  enum_errors(errors, error_min_bit, error_max_bit, "EC", add_message);

  if (warning_min_bit != warning_max_bit) {
    enum_errors(errors, warning_min_bit, warning_max_bit, "WS", add_message);
  }

  return messages;
}

void TionState::dump(const char *TAG, const TionTraits &traits) const {
  if (this->errors) {
    if (traits.errors_decode) {
      const auto errors = traits.errors_decode(this->errors);
      TION_LOGW(TAG, "Breezer alert[0x%" PRIx32 "]: %s", this->errors, errors.c_str());
    }
    if (traits.errors_report) {
      traits.errors_report(this->errors);
    }
  }

  TION_DUMP(TAG, "power       : %s", ONOFF(this->power_state));
  TION_DUMP(TAG, "heater      : %s", ONOFF(this->heater_state));
  TION_DUMP(TAG, "filter_warn : %s", ONOFF(this->filter_state));
  TION_DUMP(TAG, "fan_speed   : %d", this->fan_speed);
  TION_DUMP(TAG, "target_temp : %d °C", this->target_temperature);
  TION_DUMP(TAG, "outdoor_temp: %d °C", this->outdoor_temperature);
  TION_DUMP(TAG, "current_temp: %d °C", this->current_temperature);
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

  TION_DUMP(TAG, "productivity: %u m³", this->productivity);

  TION_DUMP(TAG, "filter_time : %" PRIu32 " s", this->filter_time_left);
  if (traits.supports_work_time) {
    TION_DUMP(TAG, "work_time   : %" PRIu32 " s", this->work_time);
  }
  if (traits.supports_fan_time) {
    TION_DUMP(TAG, "fan_time    : %" PRIu32 " s", this->fan_time);
  }
  if (traits.supports_airflow_counter) {
    TION_DUMP(TAG, "airflow     : %.3f m³ (%" PRIu32 ")", this->airflow_m3, this->airflow_counter);
  }

  if (traits.supports_pcb_ctl_temperature) {
    TION_DUMP(TAG, "pcb_ctl_temp: %d °C", this->pcb_ctl_temperature);
  }
  if (traits.supports_pcb_pwr_temperature) {
    TION_DUMP(TAG, "pcb_pwr_temp: %d °C", this->pcb_pwr_temperature);
  }

  if (this->firmware_version) {
    TION_DUMP(TAG, "firmware_ver: %04X", this->firmware_version);
  }
  if (this->hardware_version) {
    TION_DUMP(TAG, "hardware_ver: %04X", this->hardware_version);
  }

  TION_DUMP(TAG, "errors      : 0x%08" PRIX32, this->errors);
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
  if (this->gate_error_state) {
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
  // текущее состояние
  const auto &cs = this->state_;
  // новое состояние
  auto ns = this->state_;

  if (call->get_auto_state().has_value()) {
    const auto auto_state = *call->get_auto_state();
    if (cs.auto_state != auto_state) {
      if (auto_state && !this->auto_is_valid()) {
        TION_LOGW(TAG, "Auto is not configured properly");
        ns.auto_state = false;
      } else {
        TION_LOGD(TAG, "New auto state %s -> %s", ONOFF(cs.auto_state), ONOFF(auto_state));
        ns.auto_state = auto_state;
      }
      if (ns.auto_state) {
        // автоматический режим работает только с забором воздуха снаружи
        call->set_gate_position(TionGatePosition::OUTDOOR);
      }
    }
  }

  ns.comm_source = ns.auto_state ? CommSource::AUTO : CommSource::USER;

  if (call->get_fan_speed().has_value()) {
    const auto fan_speed = *call->get_fan_speed();

    // скорость 0 поддерживается не для всех бризеров
    if (fan_speed == 0 && !this->traits_.supports_kiv) {
      // не разрешаем скорость 0, вместо этого выключаем бризер
      if (call->get_power_state().value_or(cs.power_state)) {
        // залоггируем только для не авто-режима
        if (!call->get_auto_state().value_or(false)) {
          TION_LOGW(TAG, "Zero fan speed lead to power off");
        }
        call->set_power_state(false);
      }
    } else if (fan_speed > this->traits_.max_fan_speed) {
      TION_LOGW(TAG, "Disallowed fan speed: %u", fan_speed);
    } else if (cs.fan_speed != fan_speed) {
      TION_LOGD(TAG, "New fan speed %u -> %u", cs.fan_speed, fan_speed);
      ns.fan_speed = fan_speed;

      if (!call->get_auto_state().value_or(false)) {
        // если ручное переключение скорости, то выключаем авто-режим
        ns.auto_state = false;
      }
    }
  }

  if (call->get_power_state().has_value()) {
    const auto power_state = *call->get_power_state();
    if (cs.power_state != power_state) {
      TION_LOGD(TAG, "New power state %s -> %s", ONOFF(cs.power_state), ONOFF(power_state));
      ns.power_state = power_state;
      // если ручное выключение, то выключаем авто-режим
      if (!call->get_auto_state().value_or(false) && !power_state) {
        // TODO нужно ли восстановить авто-режим при ручном включении?
        ns.auto_state = false;
        TION_LOGI(TAG, "Auto is turned off by manual power off");
      }
    }
  }

  if (call->get_heater_state().has_value()) {
    const auto heater_state = *call->get_heater_state();
    if (cs.heater_state != heater_state) {
      TION_LOGD(TAG, "New heater state %s -> %s", ONOFF(cs.heater_state), ONOFF(heater_state));
      ns.heater_state = heater_state;
    }
  }

  if (call->get_target_temperature().has_value()) {
    const auto target_temperature = *call->get_target_temperature();
    if (cs.target_temperature != target_temperature) {
      if (target_temperature < this->traits_.min_target_temperature ||
          target_temperature > this->traits_.max_target_temperature) {
        TION_LOGW(TAG, "Target temperature is out of range %d:%d °C", this->traits_.min_target_temperature,
                  this->traits_.max_target_temperature);
      } else {
        TION_LOGD(TAG, "New target temperature %d -> %d", cs.target_temperature, target_temperature);
        ns.target_temperature = target_temperature;
      }
    }
  }

  if (this->traits_.supports_sound_state) {
    if (call->get_sound_state().has_value()) {
      const auto sound_state = *call->get_sound_state();
      if (cs.sound_state != sound_state) {
        TION_LOGD(TAG, "New sound state %s -> %s", ONOFF(cs.sound_state), ONOFF(sound_state));
        ns.sound_state = sound_state;
      }
    }
  }

  if (this->traits_.supports_led_state) {
    if (call->get_led_state().has_value()) {
      const auto led_state = *call->get_led_state();
      if (cs.led_state != led_state) {
        TION_LOGD(TAG, "New led state %s -> %s", ONOFF(cs.led_state), ONOFF(led_state));
        ns.led_state = led_state;
      }
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
          break;
        }
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
        ns.gate_position = gate_position;

        // если ручное переключение, то выключаем авто-режим
        if (!call->get_auto_state().value_or(false) && gate_position != TionGatePosition::OUTDOOR) {
          // TODO нужно ли восстановить авто-режим при ручном включении?
          ns.auto_state = false;
          TION_LOGI(TAG, "Auto is turned off by manual gate position change");
        }
      }
    }
  }

  if (this->traits_.supports_manual_antifreeze) {
    if (ns.power_state && !ns.heater_state && ns.outdoor_temperature < 0) {
      TION_LOGW(TAG, "Antifreeze protection has worked, heater now enabled");
      ns.heater_state = true;
    }
  }

  if (this->traits_.supports_kiv && ns.power_state && ns.fan_speed == 0) {
    // для предотвращения обморожения не разрешаем работу в режиме kiv если внешняя температура менее 5 °C
    if (this->state_.outdoor_temperature < 5) {
      ns.power_state = false;
      TION_LOGW(TAG, "KIV mode not supported when outdoor temperature less than 5 °C");
    } else
      // не разрешаем работу в режиме kiv если включен обогреватель
      if (ns.heater_state) {
        ns.power_state = false;
        TION_LOGW(TAG, "KIV mode not supported when heater is on");
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
  if (this->auto_state_.has_value()) {
    TION_DUMP(TAG, "  auto    : %s", ONOFF(*this->auto_state_));
  }
  if (this->sound_state_.has_value()) {
    TION_DUMP(TAG, "  sound   : %s", ONOFF(*this->sound_state_));
  }
  if (this->led_state_.has_value()) {
    TION_DUMP(TAG, "  led     : %s", ONOFF(*this->led_state_));
  }
}

void TionStateCall::perform() {
  if (this->has_changes()) {
    this->dump();
    this->api_->write_state(this);
    this->reset();
  } else {
    TION_LOGV(TAG, "No changes to perform");
  }
}

bool TionStateCall::has_changes() const {
  return this->fan_speed_.has_value()              //-//
         || this->power_state_.has_value()         //-//
         || this->heater_state_.has_value()        //-//
         || this->target_temperature_.has_value()  //-//
         || this->sound_state_.has_value()         //-//
         || this->led_state_.has_value()           //-//
         || this->gate_position_.has_value()       //-//
         || this->auto_state_.has_value()          //-//
      ;
}

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

TionApiBase::TionApiBase()
#ifdef TION_ENABLE_PI_CONTROLLER
    : auto_pi_(TION_AUTO_KP, TION_AUTO_TI, TION_AUTO_DB)
#endif
{
  this->presets_.push_back({
      nullptr,
      {
          .target_temperature = -1,
          .heater_state = -1,
          .power_state = 1,
          .fan_speed = -1,  // будет инициализировано максимальной скоростью
          .gate_position = TionGatePosition::OUTDOOR,
          .auto_state = -1,
          .work_time = TION_BOOST_TIME,
      },
  });
}

void TionApiBase::notify_state_(uint32_t request_id) {
  TionStateCall *call = nullptr;

  if (this->is_boost_running()) {
    // если изменили скорость вентиляции или выключили бризер
    if (this->state_.get_fan_speed() != this->state_.max_fan_speed) {
      TION_LOGD(TAG, "Boost canceled by user action");
      // пересохраняем изменившиеся данные, для восстановления
      this->boost_save_state_();
      if (call == nullptr) {
        call = new TionStateCall(this);
      }
      this->boost_cancel_(call);  // TODO убедиться что пресет сбросится далее
    } else {
      auto time_left = this->get_boost_time_left();
      // только если натив буст не поддерживается
      if (!this->traits_.supports_boost && time_left == 0) {
        if (call == nullptr) {
          call = new TionStateCall(this);
        }
        this->boost_cancel_(call);  // TODO убедиться что пресет сбросится далее
      }
      TION_DUMP(TAG, "Boost time left %d s", time_left);
    }
  }

  if (this->activate_preset_) {
    if (!this->activate_preset_->data.is_modified(this->state_)) {
      this->active_preset_ = this->activate_preset_;
      this->activate_preset_ = nullptr;
    }
  } else if (this->active_preset_) {
    if (this->active_preset_->data.is_modified(this->state_)) {
      this->active_preset_ = nullptr;
    }
  }

  if (this->traits_.supports_manual_antifreeze) {
    const auto &cs = this->state_;
    if (cs.power_state && !cs.heater_state && cs.outdoor_temperature < 0) {
      TION_LOGW(TAG, "Antifreeze protection has worked, heater now enabled");
      if (call == nullptr) {
        call = new TionStateCall(this);
      }
      call->set_heater_state(true);
    }
  }

  if (call) {
    call->perform();
    delete call;
  }

  if (this->on_state_) {
    this->on_state_(this->state_, request_id);
  }
}

void TionApiBase::set_boost_time(uint16_t boost_time) {
  if (boost_time > 0) {
    TION_LOGD(TAG, "New boost time: %u s", boost_time);
    this->boost_preset_().data.work_time = boost_time;
  }
}

uint16_t TionApiBase::get_boost_time_left() const {
  if (this->is_boost_running()) {
    const auto work_time = this->state_.work_time - this->boost_save_.start_time;
    auto &preset = this->boost_preset_().data;
    if (work_time < preset.work_time) {
      return preset.work_time - work_time;
    }
  }
  return 0;
}

void TionApiBase::set_boost_heater_state(bool heater_state) {
  const int8_t st = heater_state ? 1 : 0;
  TION_LOGD(TAG, "New boost heater state: %s", ONOFF(heater_state));
  this->boost_preset_().data.heater_state = st;
}

void TionApiBase::set_boost_target_temperature(int8_t target_temperature) {
  if (target_temperature < this->traits_.min_target_temperature ||
      target_temperature > this->traits_.max_target_temperature) {
    TION_LOGW(TAG, "Boost target temperature is out of range %d:%d °C", this->traits_.min_target_temperature,
              this->traits_.max_target_temperature);
    return;
  }
  TION_LOGD(TAG, "New boost target temperature: %d °C", target_temperature);
  this->boost_preset_().data.target_temperature = target_temperature;
}

void TionApiBase::enable_boost(bool state, TionStateCall *call) {
  if (call == nullptr) {
    INVALID_STATE_CALL();
    return;
  }

  if (!this->state_.is_initialized()) {
    TION_LOGW(TAG, "State was not initialized");
    return;
  }

  TION_LOGD(TAG, "Switching boost to %s", ONOFF(state));
  const uint16_t new_boost_time = state ? this->boost_preset_().data.work_time : 0U;
  this->enable_boost(new_boost_time, call);
}

void TionApiBase::enable_boost(uint16_t boost_time, TionStateCall *call) {
  if (call == nullptr) {
    INVALID_STATE_CALL();
    return;
  }

  if (!this->state_.is_initialized()) {
    TION_LOGW(TAG, "State was not initialized");
    return;
  }

  if (boost_time > 0) {
    this->boost_enable_(boost_time, call);
  } else {
    this->boost_cancel_(call);
  }
}

void TionApiBase::boost_enable_(uint16_t boost_time, TionStateCall *call) {
  if (this->is_boost_running()) {
    TION_LOGW(TAG, "Boost is already in progress, time left %u s", this->get_boost_time_left());
    return;
  }

  if (this->state_.get_fan_speed() == this->state_.max_fan_speed) {
    TION_LOGW(TAG, "Fan is already running at maximum speed");
    return;
  }

  if (boost_time == 0) {
    TION_LOGW(TAG, "Boost time is not configured");
    return;
  }

  TION_LOGD(TAG, "Schedule boost for %d s", boost_time);
  this->boost_save_.start_time = this->state_.work_time;
  this->boost_save_state_();
  // скорость может быть переопределена пресетом
  call->set_fan_speed(this->state_.max_fan_speed);
  this->boost_preset_().data.to_call(call);
  // дополнительно оставим авто-режим в текущем положении
  call->set_auto_state(this->state_.auto_state);
}

void TionApiBase::boost_save_state_() { this->boost_save_.from_state(this->state_); }

void TionApiBase::PresetData::from_state(const TionState &state) {
  this->power_state = state.power_state;
  this->heater_state = state.heater_state;
  this->fan_speed = state.fan_speed;
  this->target_temperature = state.target_temperature;
  this->gate_position = state.gate_position;
  this->auto_state = state.auto_state;
}

void TionApiBase::PresetData::to_call(TionStateCall *call) const {
  if (call == nullptr) {
    INVALID_STATE_CALL();
    return;
  }
  if (this->has_power_state()) {
    call->set_power_state(this->power_state > 0);
  }
  if (this->has_heater_state()) {
    call->set_heater_state(this->heater_state > 0);
  }
  if (this->has_fan_speed()) {
    call->set_fan_speed(this->fan_speed);
  }
  if (this->has_target_temperature()) {
    call->set_target_temperature(this->target_temperature);
  }
  if (this->has_gate_position()) {
    call->set_gate_position(this->gate_position);
  }
  if (this->has_auto_state()) {
    call->set_auto_state(this->auto_state > 0);
  }
}

bool TionApiBase::PresetData::is_filled() const {
  return this->has_power_state() ||         //
         this->has_fan_speed() ||           //
         this->has_heater_state() ||        //
         this->has_target_temperature() ||  //
         this->has_auto_state() ||          //
         this->has_gate_position();
}

bool TionApiBase::PresetData::is_valid(const char *name, const TionTraits &traits) const {
  if (!this->is_filled()) {
    TION_LOGW(TAG, "Preset '%s' has no data to change", name);
    return false;
  }
  if (this->has_target_temperature() && this->target_temperature < traits.min_target_temperature) {
    TION_LOGW(TAG, "Preset '%s' has invalid target temperature %d", name, this->target_temperature);
    return false;
  }
  if (this->fan_speed > traits.max_fan_speed) {
    TION_LOGW(TAG, "Preset '%s' has invalid fan speed %u", name, this->fan_speed);
    return false;
  }
  return true;
}

bool TionApiBase::PresetData::is_modified(const TionState &st) const {
  if (this->has_power_state() && this->power_state != st.power_state) {
    TION_LOGV(TAG, "Preset is modified by %s: %d vs %d", "power", st.power_state, this->power_state);
    return true;
  }
  if (this->has_heater_state() && this->heater_state != st.heater_state) {
    TION_LOGV(TAG, "Preset is modified by %s: %d vs %d", "heat", st.heater_state, this->heater_state);
    return true;
  }
  if (this->has_fan_speed() && this->fan_speed != st.fan_speed) {
    TION_LOGV(TAG, "Preset is modified by %s: %u vs %d", "fan", st.fan_speed, this->fan_speed);
    return true;
  }
  if (this->has_target_temperature() && this->target_temperature != st.target_temperature) {
    TION_LOGV(TAG, "Preset is modified by %s: %d vs %d", "temp", st.target_temperature, this->target_temperature);
    return true;
  }
  if (this->has_gate_position() && this->gate_position != st.gate_position) {
    TION_LOGV(TAG, "Preset is modified by %s: %d vs %d", "gate", int(st.gate_position), this->gate_position);
    return true;
  }
  if (this->has_auto_state() && this->auto_state != st.auto_state) {
    TION_LOGV(TAG, "Preset is modified by %s: %d vs %d", "auto", st.auto_state, this->auto_state);
    return true;
  }
  TION_LOGV(TAG, "Preset was not modified");
  return false;
}

void TionApiBase::boost_cancel_(TionStateCall *call) {
  if (!this->is_boost_running()) {
    return;
  }
  TION_LOGD(TAG, "Boost finished");
  this->boost_save_.start_time = 0;
  this->boost_save_.to_call(call);
}

void TionApiBase::enable_preset(const char *preset, TionStateCall *call) {
  TION_LOGD(TAG, "Activate preset '%s'", preset);
  if (preset == nullptr || *preset == 0 || strcasecmp(preset, PRESET_NONE) == 0) {
    if (this->active_preset_) {
      if (this->active_preset_->has_timer()) {
        this->enable_boost(false, call);
      }
      this->active_preset_ = nullptr;
    }
    this->activate_preset_ = nullptr;
    return;
  }

  for (auto &&it : this->presets_) {
    if (it.name && strcasecmp(preset, it.name) == 0) {
      this->activate_preset_ = &it;
      this->active_preset_ = nullptr;
      if (it.has_timer()) {
        this->enable_boost(true, call);
      } else {
        it.data.to_call(call);
      }
      return;
    }
  }

  TION_LOGD(TAG, "Preset '%s' not found", preset);
}

std::vector<const char *> TionApiBase::get_presets() const {
  std::vector<const char *> presets;
  presets.push_back(PRESET_NONE);
  for (auto &&preset : this->presets_) {
    if (preset.name) {
      presets.push_back(preset.name);
    }
  }
  return presets;
};

void TionApiBase::add_preset(const char *name, const PresetData &data) {
  if (name == nullptr || *name == 0) {
    TION_LOGW(TAG, "Empty preset name");
    return;
  }
  if (strcasecmp(name, PRESET_NONE) == 0) {
    TION_LOGW(TAG, "Skip reserved preset '%s'", name);
    return;
  }
  if (!data.is_valid(name, this->traits_)) {
    return;
  }
  if (data.has_timer()) {
    TION_LOGD(TAG, "Update preset '%s': heat=%d, fan=%u, temp=%d,", name, data.heater_state, data.fan_speed,
              data.target_temperature);
    auto &preset = this->boost_preset_();
    preset.name = name;
    preset.data.target_temperature = data.target_temperature;
    preset.data.heater_state = data.heater_state;
    if (data.fan_speed > 0) {
      preset.data.fan_speed = data.fan_speed;
    } else if (preset.data.fan_speed <= 0) {
      preset.data.fan_speed = this->traits_.max_fan_speed;
    }
  } else {
    TION_LOGD(TAG, "Setup preset '%s': power=%d, heat=%d, fan=%u, temp=%d, gate=%u", name, data.power_state,
              data.heater_state, data.fan_speed, data.target_temperature, static_cast<uint8_t>(data.gate_position));
    this->presets_.push_back({.name = name, .data = data});
  }
}

#ifdef TION_ENABLE_PI_CONTROLLER
void TionApiBase::set_auto_pi_data(float kp, float ti, int db) {
  if (kp > 0 && ti > 0) {
    this->auto_pi_.reset(kp, ti, db);
  } else {
    TION_LOGW(TAG, "Invalid Kp=%.04f or Ti=%.04f", kp, ti);
  }
}
#endif

void TionApiBase::set_auto_min_fan_speed(uint8_t auto_min_fan_speed) {
  if (auto_min_fan_speed > this->traits_.max_fan_speed - 1) {
    TION_LOGW(TAG, "Invalid min fan speed %u", auto_min_fan_speed);
    return;
  }
  this->auto_min_fan_speed_ = auto_min_fan_speed;
  TION_LOGD(TAG, "New auto min fan speed: %u", this->auto_min_fan_speed_);

  if (auto_min_fan_speed >= this->auto_max_fan_speed_) {
    this->auto_max_fan_speed_ = auto_min_fan_speed + 1;
    TION_LOGD(TAG, "Fix auto max fan speed: %u", this->auto_max_fan_speed_);
  }

  this->auto_update_fan_speed_();
}

void TionApiBase::set_auto_max_fan_speed(uint8_t auto_max_fan_speed) {
  if (auto_max_fan_speed < 1 || auto_max_fan_speed > this->traits_.max_fan_speed) {
    TION_LOGW(TAG, "Invalid max fan speed %u", auto_max_fan_speed);
    return;
  }

  this->auto_max_fan_speed_ = auto_max_fan_speed;
  TION_LOGD(TAG, "New auto max fan speed: %u", this->auto_max_fan_speed_);

  if (auto_max_fan_speed <= this->auto_min_fan_speed_) {
    this->auto_min_fan_speed_ = auto_max_fan_speed - 1;
    TION_LOGD(TAG, "Fix auto min fan speed: %u", this->auto_min_fan_speed_);
  }

  this->auto_update_fan_speed_();
}

void TionApiBase::auto_update_fan_speed_() {
#ifdef TION_ENABLE_PI_CONTROLLER
  this->auto_pi_.set_min(this->traits_.auto_prod[this->auto_min_fan_speed_]);
  this->auto_pi_.set_max(this->traits_.auto_prod[this->auto_max_fan_speed_]);
#endif
  this->auto_reset();
}

void TionApiBase::set_auto_setpoint(uint16_t setpoint) {
  this->auto_setpoint_ = setpoint;
  this->auto_reset();
}

void TionApiBase::auto_reset() {
  TION_LOGD(TAG, "Auto update settings min: %u, max: %u, setpoint: %u", this->auto_min_fan_speed_,
            this->auto_max_fan_speed_, this->auto_setpoint_);
  // сначала уведомим все сущности об изменениях
  if (this->on_state_) {
    this->on_state_(this->state_, 0);
  }
  // потом уведомим авто-режим
  // здесь не проверяем auto_state, чтобы когда он включиться все уже было готово
  if (this->auto_update_func_) {
    this->auto_update_func_(0);
  } else {
#ifdef TION_ENABLE_PI_CONTROLLER
    this->auto_pi_.reset();
#endif
  }
}

bool TionApiBase::auto_update(uint16_t current, TionStateCall *call) {
  if (!this->state_.auto_state) {
    return false;
  }

  // режим турбо имеет приоритет
  if (this->is_boost_running()) {
    return false;
  }

  if (call == nullptr) {
    INVALID_STATE_CALL();
    return false;
  }

  if (current < 400) {
    TION_LOGD(TAG, "Invalid co2 level: %u", current);
    return false;
  }

  uint8_t fan_speed = this->state_.get_fan_speed();

  TION_LOGD(TAG, "Auto cur fan speed %u, new co2 %u ppm", fan_speed, current);

  if (this->auto_update_func_) {
    fan_speed = this->auto_update_func_(current);
    if (fan_speed < this->auto_min_fan_speed_) {
      fan_speed = this->auto_min_fan_speed_;
    } else if (fan_speed > this->auto_max_fan_speed_) {
      fan_speed = this->auto_max_fan_speed_;
    }
  } else {
#ifdef TION_ENABLE_PI_CONTROLLER
    fan_speed = this->auto_pi_update_(current);
#endif
  }

  if (fan_speed == this->state_.get_fan_speed()) {
    return false;
  }

  TION_LOGD(TAG, "Auto new fan speed %u", fan_speed);
  // для понимания, что переключение было из авто-режима, всегда выставляем авто
  call->set_auto_state(true);
  call->set_fan_speed(fan_speed);
  if (fan_speed > 0) {
    call->set_power_state(true);
  }
  return true;
}

#ifdef TION_ENABLE_PI_CONTROLLER
uint8_t TionApiBase::auto_pi_update_(uint16_t current) {
  int rate = this->auto_pi_.update(this->auto_setpoint_, current);
  TION_LOGV(TAG, "Auto PI rate: %d", rate);
  if (rate > 0) {
    // приводим m^3/h в скорость вентиляции
    for (auto i = this->traits_.max_fan_speed; i > 0; i--) {
      int test = this->traits_.auto_prod[i - 1];
      if (rate > test) {
        // отсечем нижний предел
        if (i < this->auto_min_fan_speed_) {
          return this->auto_min_fan_speed_;
        }
        // отсечем верхний предел
        if (i > this->auto_max_fan_speed_) {
          return this->auto_max_fan_speed_;
        }
        // gotcha
        return i;
      }
    }
  }
  // не нашли подходящего значения, работаем по-минимуму
  return this->auto_min_fan_speed_;
}
#endif

bool TionApiBase::auto_is_valid() const {
  return !!this->auto_update_func_ ||
         (this->auto_setpoint_ > 400 && this->auto_min_fan_speed_ < this->auto_max_fan_speed_);
}

}  // namespace tion
}  // namespace dentra
