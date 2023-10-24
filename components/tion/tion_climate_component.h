#pragma once

#include "esphome/core/defines.h"

#ifdef USE_CLIMATE
#include "esphome/core/helpers.h"

#if defined(TION_ENABLE_PRESETS) && defined(USE_API)
#define TION_ENABLE_PRESETS_WITH_API
#endif

#ifdef TION_ENABLE_PRESETS_WITH_API
#include "esphome/components/api/custom_api_device.h"
#endif  // TION_ENABLE_PRESETS_WITH_API

#include "../tion-api/tion-api.h"

#include "tion_component.h"
#include "tion_climate.h"
#include "tion_vport.h"

namespace esphome {
namespace tion {

#ifdef TION_ENABLE_PRESETS_WITH_API
using TionApiDevice = api::CustomAPIDevice;
#else
class TionApiDevice {};
#endif  // TION_ENABLE_PRESETS_WITH_API

class TionClimateComponentBase : public TionClimate, public TionComponent, public TionApiDevice {
 public:
  TionClimateComponentBase(TionVPortType vport_type) : vport_type_(vport_type) {}
  void call_setup() override;
  void dump_settings(const char *tag, const char *component) const;

 protected:
  const TionVPortType vport_type_;

#ifdef TION_ENABLE_PRESETS
  bool enable_boost_() override;
  void cancel_boost_() override;
  /// returns boost time in seconds.
  uint32_t get_boost_time_() const {
    if (this->boost_time_ == nullptr) {
      return DEFAULT_BOOST_TIME_SEC;
    }
    if (this->boost_time_->traits.get_unit_of_measurement()[0] == 's') {
      return this->boost_time_->state;
    }
    return this->boost_time_->state * 60;
  }

  void update_preset_service_(std::string preset, std::string mode, int fan_speed, int target_temperature);
#endif
#ifdef TION_ENABLE_PRESETS_WITH_API
  ESPPreferenceObject rtc_;
#endif
};  // namespace tion

/**
 * @param tion_api_type TionApi implementation.
 * @param tion_state_type Tion state struct.
 */
template<class tion_api_type> class TionClimateComponent : public TionClimateComponentBase {
  using tion_state_type = typename tion_api_type::state_type;

  static_assert(std::is_base_of<dentra::tion::TionApiBase<tion_state_type>, tion_api_type>::value,
                "tion_api_type is not derived from TionApiBase");

 public:
  explicit TionClimateComponent(tion_api_type *api, TionVPortType vport_type)
      : TionClimateComponentBase(vport_type), api_(api) {
    using this_t = typename std::remove_pointer<decltype(this)>::type;
    this->api_->on_dev_info.template set<this_t, &this_t::on_dev_info>(*this);
    this->api_->on_state.template set<this_t, &this_t::on_state>(*this);
    this->api_->set_on_ready(tion_api_type::on_ready_type::template create<this_t, &this_t::on_ready>(*this));
  }

  void update() override { this->api_->request_state(); }

  void on_ready() {
    if (!this->state_.is_initialized()) {
      this->api_->request_dev_info();
    }
  }

  virtual void update_state(const tion_state_type &state) = 0;

  void on_state(const tion_state_type &state, const uint32_t request_id) {
    this->update_state(state);

    if (this->state_warnout_ && this->state_timeout_ > 0) {
      this->state_warnout_->publish_state(false);
      this->cancel_timeout("state_timeout");
    }
    if (this->outdoor_temperature_) {
      this->outdoor_temperature_->publish_state(state.outdoor_temperature);
    }
    if (this->buzzer_) {
      this->buzzer_->publish_state(state.flags.sound_state);
    }
    if (this->filter_time_left_) {
      this->filter_time_left_->publish_state(state.counters.filter_time_left());
    }
    if (this->filter_warnout_) {
      this->filter_warnout_->publish_state(state.filter_warnout());
    }

    if (!this->batch_active_) {
      this->state_ = state;
    }
  }

  void on_dev_info(const dentra::tion::tion_dev_info_t &status) { this->update_dev_info_(status); }

 protected:
  tion_api_type *api_;
  tion_state_type state_{};
  uint32_t request_id_{};
  bool batch_active_{};
  void write_api_state_(const tion_state_type &state) {
    ESP_LOGV("tion_climate_component", "%s batch update", this->batch_active_ ? "Continue" : "Starting");
    this->batch_active_ = true;
    this->state_ = state;
    this->set_timeout("batch_update", this->batch_timeout_, [this]() {
      ESP_LOGV("tion_climate_component", "Write out batch changes");
      this->api_->write_state(this->state_, ++this->request_id_);
      if (this->state_warnout_ && this->state_timeout_ > 0) {
        this->set_timeout("state_timeout", this->state_timeout_,
                          [this]() { this->state_warnout_->publish_state(true); });
      }
      this->batch_active_ = false;
    });
  }
};

template<class tion_api_type> class TionLtClimateComponent : public TionClimateComponent<tion_api_type> {
  using tion_state_type = typename tion_api_type::state_type;

 public:
  explicit TionLtClimateComponent(tion_api_type *api, TionVPortType vport_type)
      : TionClimateComponent<tion_api_type>(api, vport_type) {
    using this_t = typename std::remove_pointer<decltype(this)>::type;
    this->api_->on_state.template set<this_t, &this_t::on_state>(*this);
  }

  void on_state(const tion_state_type &state, const uint32_t request_id) {
    // this->state_ will set to new state in TionClimateComponent::on_state,
    // so save some vars here
    auto prev_airflow_counter = this->state_.counters.airflow_counter;
    auto prev_fan_time = this->state_.counters.fan_time;

    TionClimateComponent<tion_api_type>::on_state(state, request_id);

    if (this->led_) {
      this->led_->publish_state(state.flags.led_state);
    }
    if (this->heater_power_) {
      this->heater_power_->publish_state(state.heater_power());
    }
    if (this->airflow_counter_) {
      this->airflow_counter_->publish_state(state.counters.airflow());
    }
    if (this->productivity_ && prev_fan_time != 0) {
      auto diff_time = state.counters.fan_time - prev_fan_time;
      if (diff_time == 0) {
        this->productivity_->publish_state(0);
      } else {
        auto diff_airflow = state.counters.airflow_counter - prev_airflow_counter;
        this->productivity_->publish_state(float(diff_airflow) / float(diff_time) * float(state.counters.airflow_k()));
      }
    }
  }
};

class TionBoostTimeNumber : public number::Number {
 public:
 protected:
  void control(float value) override { this->publish_state(value); }
};

}  // namespace tion
}  // namespace esphome
#endif
