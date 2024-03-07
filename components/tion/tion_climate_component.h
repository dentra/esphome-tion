#pragma once
#include "esphome/core/defines.h"
#ifdef USE_CLIMATE

#include "esphome/core/helpers.h"
#include "esphome/components/climate/climate.h"

#include "../tion-api/tion-api.h"

#include "tion_component.h"
#include "tion_climate_presets.h"
#include "tion_vport.h"
#include "tion_batch_call.h"

namespace esphome {
namespace tion {

class TionClimateComponentBase : public climate::Climate, public TionClimatePresets, public TionComponent {
 public:
  TionClimateComponentBase() = delete;
  TionClimateComponentBase(const TionClimateComponentBase &) = delete;             // non construction-copyable
  TionClimateComponentBase &operator=(const TionClimateComponentBase &) = delete;  // non copyable

  TionClimateComponentBase(TionVPortType vport_type);

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
  virtual bool enable_boost() { return this->presets_enable_boost_timer_(this, this); }
  virtual void cancel_boost() { this->presets_cancel_boost_timer_(this); }
#endif
};

/**
 * @param tion_api_type TionApi implementation.
 */
template<class tion_api_type> class TionClimateComponent : public TionClimateComponentBase {
  static_assert(std::is_base_of_v<dentra::tion::TionApiBase, tion_api_type>,
                "tion_api_type is not derived from TionApiBase");

 public:
  explicit TionClimateComponent(tion_api_type *api, TionVPortType vport_type)
      : TionClimateComponentBase(vport_type), api_(api) {
    using this_t = std::remove_pointer_t<decltype(this)>;
    this->api_->on_dev_info_fn.template set<this_t, &this_t::on_dev_info>(*this);
    this->api_->on_state_fn.template set<this_t, &this_t::on_state>(*this);
  }

  void update() override {
    this->api_->request_state();
    this->schedule_state_check_();
  }

  virtual void update_state(const dentra::tion::TionState &state) = 0;

  void on_state(const dentra::tion::TionState &state, const uint32_t request_id) {
    this->update_state(state);

    this->cancel_state_check_();
#ifdef USE_TION_OUTDOOR_TEMPERATURE
    if (this->outdoor_temperature_) {
      this->outdoor_temperature_->publish_state(state.outdoor_temperature);
    }
#endif
#ifdef USE_TION_BUZZER
    if (this->buzzer_) {
      this->buzzer_->publish_state(state.sound_state);
    }
#endif
#ifdef USE_TION_FILTER_TIME_LEFT
    if (this->filter_time_left_) {
      this->filter_time_left_->publish_state(state.filter_time_left_d());
    }
#endif
#ifdef USE_TION_FILTER_WARNOUT
    if (this->filter_warnout_) {
      this->filter_warnout_->publish_state(state.filter_state);
    }
#endif
#ifdef USE_TION_WORK_TIME
    if (this->work_time_) {
      this->work_time_->publish_state(state.work_time);
    }
#endif
#ifdef USE_TION_ERRORS
    if (this->errors_) {
      this->errors_->publish_state(this->api_->traits().errors_decoder(state.errors));
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
  dentra::tion::TionState state_{};
  uint32_t request_id_{};
  bool batch_active_{};

  BatchStateCall *state_call_{};

  BatchStateCall *make_api_call() {
    if (this->state_call_ == nullptr) {
      state_call_ = new BatchStateCall(this->api_, this, this->batch_timeout_, [this]() {
        this->state_call_->reset();
        this->schedule_state_check_();
      });
    }
    return this->state_call_;
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
 public:
  explicit TionLtClimateComponent(tion_api_type *api, TionVPortType vport_type)
      : TionClimateComponent<tion_api_type>(api, vport_type) {
    using this_t = std::remove_pointer_t<decltype(this)>;
    this->api_->on_state_fn.template set<this_t, &this_t::on_state>(*this);
  }

  void on_state(const dentra::tion::TionState &state, const uint32_t request_id) {
    TionClimateComponent<tion_api_type>::on_state(state, request_id);
#ifdef USE_TION_LED
    if (this->led_) {
      this->led_->publish_state(state.led_state);
    }
#endif
#ifdef USE_TION_HEATER_POWER
    if (this->heater_power_) {
      this->heater_power_->publish_state(state.get_heater_power(this->api_->traits()));
    }
#endif
#ifdef USE_TION_AIRFLOW_COUNTER
    if (this->airflow_counter_) {
      this->airflow_counter_->publish_state(state.counters.airflow());
    }
#endif
#ifdef USE_TION_PRODUCTIVITY
    if (this->productivity_) {
      this->productivity_->publish_state(state.productivity);
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
