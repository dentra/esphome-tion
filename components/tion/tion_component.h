#pragma once
#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/number/number.h"
#include "esphome/components/button/button.h"

#include "../tion-api/tion-api.h"

namespace esphome {
namespace tion {
class TionComponent : public PollingComponent {
 public:
  void call_setup() override;

  void set_version(text_sensor::TextSensor *version) { this->version_ = version; }
  void set_buzzer(switch_::Switch *buzzer) { this->buzzer_ = buzzer; }
  void set_led(switch_::Switch *led) { this->led_ = led; }
  void set_outdoor_temperature(sensor::Sensor *outdoor_temperature) {
    this->outdoor_temperature_ = outdoor_temperature;
  }
  void set_heater_power(sensor::Sensor *heater_power) { this->heater_power_ = heater_power; }
  void set_airflow_counter(sensor::Sensor *airflow_counter) { this->airflow_counter_ = airflow_counter; }
  void set_productivity(sensor::Sensor *productivity) { this->productivity_ = productivity; }
  void set_filter_time_left(sensor::Sensor *filter_time_left) { this->filter_time_left_ = filter_time_left; }
  void set_filter_warnout(binary_sensor::BinarySensor *filter_warnout) { this->filter_warnout_ = filter_warnout; }

#ifdef TION_ENABLE_PRESETS
  void set_boost_time(number::Number *boost_time) { this->boost_time_ = boost_time; }
  void set_boost_time_left(sensor::Sensor *boost_time_left) { this->boost_time_left_ = boost_time_left; }
#endif
  void set_reset_filter(button::Button *reset_filter) { this->reset_filter_ = reset_filter; };
  void set_reset_filter_confirm(switch_::Switch *reset_filter_confirm) {
    this->reset_filter_confirm_ = reset_filter_confirm;
  };

  void set_state_warnout(binary_sensor::BinarySensor *state_warnout) { this->state_warnout_ = state_warnout; };
  void set_state_timeout(uint32_t state_timeout) { this->state_timeout_ = state_timeout; };
  void set_batch_timeout(uint32_t batch_timeout) { this->batch_timeout_ = batch_timeout; };

  void set_errors(sensor::Sensor *errors) { this->errors_ = errors; }

  bool is_reset_filter_confirmed() const {
    return this->reset_filter_confirm_ == nullptr || this->reset_filter_confirm_->state;
  }

 protected:
  text_sensor::TextSensor *version_{};
  switch_::Switch *buzzer_{};
  switch_::Switch *led_{};

  sensor::Sensor *outdoor_temperature_{};
  sensor::Sensor *heater_power_{};
  sensor::Sensor *airflow_counter_{};
  sensor::Sensor *productivity_{};
  sensor::Sensor *filter_time_left_{};
  binary_sensor::BinarySensor *filter_warnout_{};

  button::Button *reset_filter_{};
  switch_::Switch *reset_filter_confirm_{};
  binary_sensor::BinarySensor *state_warnout_{};
  uint32_t state_timeout_{};
  uint32_t batch_timeout_{};
  sensor::Sensor *errors_{};

#ifdef TION_ENABLE_PRESETS
  number::Number *boost_time_{};
  sensor::Sensor *boost_time_left_{};
  ESPPreferenceObject boost_rtc_;
#endif

  void update_dev_info_(const dentra::tion::tion_dev_info_t &info);
};
}  // namespace tion
}  // namespace esphome
