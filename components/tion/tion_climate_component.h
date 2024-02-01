#pragma once
#include "esphome/core/defines.h"
#ifdef USE_CLIMATE

#include "esphome/core/helpers.h"
#include "esphome/components/climate/climate.h"

#include "../tion-api/tion-api.h"

#include "tion_component.h"
#include "tion_climate_presets.h"
#include "tion_vport.h"

namespace esphome {
namespace tion {

class TionClimateComponentBase : public climate::Climate, public TionClimatePresets, public TionComponent {
 public:
  TionClimateComponentBase() = delete;
  TionClimateComponentBase(const TionClimateComponentBase &) = delete;             // non construction-copyable
  TionClimateComponentBase &operator=(const TionClimateComponentBase &) = delete;  // non copyable

  TionClimateComponentBase(TionVPortType vport_type) : vport_type_(vport_type) { this->target_temperature = NAN; }
  void call_setup() override;
  void dump_settings(const char *tag, const char *component) const;

  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  virtual void control_climate_state(climate::ClimateMode mode, uint8_t fan_speed, float target_temperature,
                                     TionGatePosition gate_position) = 0;

  uint8_t get_fan_speed() const { return fan_mode_to_speed(this->custom_fan_mode); }

 protected:
  const TionVPortType vport_type_;

  void set_fan_speed_(uint8_t fan_speed);

#ifdef TION_ENABLE_PRESETS
  virtual bool enable_boost() { return this->presets_enable_boost_(this, this); }
  virtual void cancel_boost() { this->presets_cancel_boost_(this, this); }
  bool enable_preset_(climate::ClimatePreset new_preset) {
    auto *preset_data = this->presets_enable_preset_(new_preset, this, this);
    if (!preset_data) {
      return false;
    }
    this->control_climate_state(preset_data->mode, preset_data->fan_speed, preset_data->target_temperature,
                                preset_data->gate_position);
    this->preset = new_preset;
    return true;
  }
  void cancel_preset_(climate::ClimatePreset old_preset) {
    if (this->presets_cancel_preset_(old_preset, this, this)) {
      this->preset = old_preset;
    }
  }
#endif
};

/**
 * @param tion_api_type TionApi implementation.
 * @param tion_state_type Tion state struct.
 */
template<class tion_api_type> class TionClimateComponent : public TionClimateComponentBase {
  using tion_state_type = typename tion_api_type::state_type;

  static_assert(std::is_base_of_v<dentra::tion::TionApiBase<tion_state_type>, tion_api_type>,
                "tion_api_type is not derived from TionApiBase");

 public:
  explicit TionClimateComponent(tion_api_type *api, TionVPortType vport_type)
      : TionClimateComponentBase(vport_type), api_(api) {
    using this_t = std::remove_pointer_t<decltype(this)>;
    this->api_->on_dev_info.template set<this_t, &this_t::on_dev_info>(*this);
    this->api_->on_state.template set<this_t, &this_t::on_state>(*this);
    this->api_->set_on_ready(tion_api_type::on_ready_type::template create<this_t, &this_t::on_ready>(*this));
  }

  void update() override {
    this->api_->request_state();
    this->schedule_state_check_();
  }

  void on_ready() {
    if (!this->state_.is_initialized()) {
      this->api_->request_dev_info();
    }
  }

  virtual void update_state(const tion_state_type &state) = 0;

  void on_state(const tion_state_type &state, const uint32_t request_id) {
    this->update_state(state);

    this->cancel_state_check_();
#ifdef USE_TION_OUTDOOR_TEMPERATURE
    if (this->outdoor_temperature_) {
      this->outdoor_temperature_->publish_state(state.outdoor_temperature);
    }
#endif
#ifdef USE_TION_BUZZER
    if (this->buzzer_) {
      this->buzzer_->publish_state(state.flags.sound_state);
    }
#endif
#ifdef USE_TION_FILTER_TIME_LEFT
    if (this->filter_time_left_) {
      this->filter_time_left_->publish_state(state.counters.filter_time_left());
    }
#endif
#ifdef USE_TION_FILTER_WARNOUT
    if (this->filter_warnout_) {
      this->filter_warnout_->publish_state(state.filter_warnout());
    }
#endif
#ifdef USE_TION_WORK_TIME
    if (this->work_time_) {
      this->work_time_->publish_state(state.counters.work_time);
    }
#endif
    // do not update state in batch mode
    if (!this->batch_active_) {
      this->state_ = state;
    }
  }

  void on_dev_info(const dentra::tion::tion_dev_info_t &info) { this->update_dev_info_(info); }

 protected:
  tion_api_type *api_;
  tion_state_type state_{};
  uint32_t request_id_{};
  bool batch_active_{};
  void write_api_state_(const tion_state_type &state) {
    ESP_LOGD("tion_climate_component", "%s batch update for %u ms", this->batch_active_ ? "Continue" : "Starting",
             this->batch_timeout_);
    this->batch_active_ = true;
    this->state_ = state;
    this->set_timeout("batch_update", this->batch_timeout_, [this]() { this->write_batch_state_(); });
  }

  void write_batch_state_() {
    ESP_LOGD("tion_climate_component", "Write out batch changes");
    this->api_->write_state(this->state_, ++this->request_id_);
    this->batch_active_ = false;
    this->schedule_state_check_();
  }

  void schedule_state_check_() {
#ifdef USE_TION_STATE_WARNOUT
    if (this->state_warnout_ && this->state_timeout_ > 0) {
      this->set_timeout("state_timeout", this->state_timeout_, [this]() { this->state_warnout_->publish_state(true); });
    }
#endif
  }

  void cancel_state_check_() {
#ifdef USE_TION_STATE_WARNOUT
    if (this->state_warnout_ && this->state_timeout_ > 0) {
      this->state_warnout_->publish_state(false);
      this->cancel_timeout("state_timeout");
    }
#endif
  }
};

template<class tion_api_type> class TionLtClimateComponent : public TionClimateComponent<tion_api_type> {
  using tion_state_type = typename tion_api_type::state_type;

 public:
  explicit TionLtClimateComponent(tion_api_type *api, TionVPortType vport_type)
      : TionClimateComponent<tion_api_type>(api, vport_type) {
    using this_t = std::remove_pointer_t<decltype(this)>;
    this->api_->on_state.template set<this_t, &this_t::on_state>(*this);
  }

  void on_state(const tion_state_type &state, const uint32_t request_id) {
#ifdef USE_TION_PRODUCTIVITY
    // this->state_ will set to new state in TionClimateComponent::on_state,
    // so save some vars here
    auto prev_airflow_counter = this->state_.counters.airflow_counter;
    auto prev_fan_time = this->state_.counters.fan_time;
#endif

    TionClimateComponent<tion_api_type>::on_state(state, request_id);
#ifdef USE_TION_LED
    if (this->led_) {
      this->led_->publish_state(state.flags.led_state);
    }
#endif
#ifdef USE_TION_HEATER_POWER
    if (this->heater_power_) {
      this->heater_power_->publish_state(state.heater_power());
    }
#endif
#ifdef USE_TION_AIRFLOW_COUNTER
    if (this->airflow_counter_) {
      this->airflow_counter_->publish_state(state.counters.airflow());
    }
#endif
#ifdef USE_TION_PRODUCTIVITY
    if (this->productivity_ && prev_fan_time != 0) {
      auto diff_time = state.counters.fan_time - prev_fan_time;
      if (diff_time == 0) {
        this->productivity_->publish_state(0);
      } else {
        auto diff_airflow = state.counters.airflow_counter - prev_airflow_counter;
        this->productivity_->publish_state(float(diff_airflow) / float(diff_time) * float(state.counters.airflow_k()));
      }
    }
#endif
  }
};

class TionBoostTimeNumber : public number::Number {
 public:
 protected:
  void control(float value) override { this->publish_state(value); }
};

}  // namespace tion
}  // namespace esphome
#endif  // USE_CLIMATE
