#pragma once

#include "../tion-api/tion-api.h"

#include "tion_component.h"
#include "tion_climate.h"
#include "tion_vport.h"

namespace esphome {
namespace tion {

class TionClimateComponentBase : public TionClimate, public TionComponent {
 public:
  void setup() override;
  void dump_settings(const char *TAG, const char *component) const;
  void set_vport_type(TionVPortType vport_type) { this->vport_type_ = vport_type; }

 protected:
  TionVPortType vport_type_{};
#ifdef TION_ENABLE_PRESETS
  bool enable_boost_() override;
  void cancel_boost_() override;
  /// returns boost time in seconds.
  uint32_t get_boost_time() const {
    if (this->boost_time_ == nullptr) {
      return DEFAULT_BOOST_TIME_SEC;
    }
    if (this->boost_time_->traits.get_unit_of_measurement()[0] == 's') {
      return this->boost_time_->state;
    }
    return this->boost_time_->state * 60;
  }
#endif
};

/**
 * @param tion_api_type TionApi implementation.
 * @param tion_state_type Tion state struct.
 */
template<class tion_api_type, class tion_state_type> class TionClimateComponent : public TionClimateComponentBase {
  static_assert(std::is_base_of<dentra::tion::TionApiBase<tion_state_type>, tion_api_type>::value,
                "tion_api_type is not derived from TionApiBase");

 public:
  explicit TionClimateComponent(tion_api_type *api) : api_(api) {
    using this_t = typename std::remove_pointer<decltype(this)>::type;
    this->api_->on_dev_status.template set<this_t, &this_t::on_dev_status>(*this);
    this->api_->on_state.template set<this_t, &this_t::on_state>(*this);
    this->api_->set_on_ready(tion_api_type::on_ready_type::template create<this_t, &this_t::on_ready>(*this));
  }

  void update() override { this->api_->request_state(); }

  void on_ready() {
    if (!this->state_.is_initialized()) {
      this->api_->request_dev_status();
    }
  }

  void write_climate_state() override {
    // softuart при нескольких последовательных записях в это же время начинает прием и тут же валится в wdt
    // запланируем *только одно* изменение состояния
    this->set_timeout("write_climate_state", 500, [this]() {
      this->publish_state();
      // write state only after first stave received (target temp will not nan)
      if (!std::isnan(this->current_temperature)) {
        this->flush_state();
      }
    });
  }

  virtual void update_state() = 0;
  virtual void dump_state() const = 0;
  virtual void flush_state() = 0;

  void on_state(const tion_state_type &state, const uint32_t request_id) {
    this->state_ = state;
    this->update_state();
#if TION_LOG_LEVEL >= TION_LOG_LEVEL_VERBOSE
    this->dump_state();
#endif
  }

  void on_dev_status(const dentra::tion::tion_dev_status_t &status) { this->update_dev_status_(status); }

 protected:
  tion_api_type *api_;
  tion_state_type state_{};
};

class TionBoostTimeNumber : public number::Number {
 public:
 protected:
  virtual void control(float value) { this->publish_state(value); }
};

class TionSwitch : public switch_::Switch {
 public:
  explicit TionSwitch(TionClimate *parent) : parent_(parent) {}
  void write_state(bool state) override {
    this->publish_state(state);
    this->parent_->write_climate_state();
  }

 protected:
  TionClimate *parent_;
};

template<class parent_t> class TionResetFilterButton : public button::Button {
 public:
  explicit TionResetFilterButton(parent_t *parent) : parent_(parent) {}
  void press_action() override { this->parent_->reset_filter(); }

 protected:
  parent_t *parent_;
};

}  // namespace tion
}  // namespace esphome
