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
#ifdef USE_TION_VERSION
  void set_version(text_sensor::TextSensor *version) { this->version_ = version; }
#endif
#ifdef USE_TION_BUZZER
  void set_buzzer(switch_::Switch *buzzer) { this->buzzer_ = buzzer; }
#endif
#ifdef USE_TION_LED
  void set_led(switch_::Switch *led) { this->led_ = led; }
#endif
#ifdef USE_TION_OUTDOOR_TEMPERATURE
  void set_outdoor_temperature(sensor::Sensor *outdoor_temperature) {
    this->outdoor_temperature_ = outdoor_temperature;
  }
#endif
#ifdef USE_TION_HEATER_POWER
  void set_heater_power(sensor::Sensor *heater_power) { this->heater_power_ = heater_power; }
#endif
#ifdef USE_TION_AIRFLOW_COUNTER
  void set_airflow_counter(sensor::Sensor *airflow_counter) { this->airflow_counter_ = airflow_counter; }
#endif
#ifdef USE_TION_PRODUCTIVITY
  void set_productivity(sensor::Sensor *productivity) { this->productivity_ = productivity; }
#endif
#ifdef USE_TION_FILTER_TIME_LEFT
  void set_filter_time_left(sensor::Sensor *filter_time_left) { this->filter_time_left_ = filter_time_left; }
#endif
#ifdef USE_TION_FILTER_WARNOUT
  void set_filter_warnout(binary_sensor::BinarySensor *filter_warnout) { this->filter_warnout_ = filter_warnout; }
#endif

#ifdef TION_ENABLE_PRESETS
  void set_boost_time(number::Number *boost_time) { this->boost_time_ = boost_time; }
  void set_boost_time_left(sensor::Sensor *boost_time_left) { this->boost_time_left_ = boost_time_left; }
#endif
#ifdef USE_TION_RESET_FILTER
  void set_reset_filter(button::Button *reset_filter) { this->reset_filter_ = reset_filter; };
  void set_reset_filter_confirm(switch_::Switch *reset_filter_confirm) {
    this->reset_filter_confirm_ = reset_filter_confirm;
  };
#endif
#ifdef USE_TION_STATE_WARNOUT
  void set_state_warnout(binary_sensor::BinarySensor *state_warnout) { this->state_warnout_ = state_warnout; };
#endif
  void set_state_timeout(uint32_t state_timeout) {
#ifdef USE_TION_STATE_WARNOUT
    this->state_timeout_ = state_timeout;
#endif
  };
  void set_batch_timeout(uint32_t batch_timeout) { this->batch_timeout_ = batch_timeout; };
#ifdef USE_TION_ERRORS
  void set_errors(text_sensor::TextSensor *errors) { this->errors_ = errors; }
#endif

#ifdef USE_TION_WORK_TIME
  void set_work_time(sensor::Sensor *work_time) { this->work_time_ = work_time; }
#endif

#ifdef USE_TION_RESET_FILTER
  bool is_reset_filter_confirmed() const {
    return this->reset_filter_confirm_ == nullptr || this->reset_filter_confirm_->state;
  }
#endif

 protected:
#ifdef USE_TION_VERSION
  text_sensor::TextSensor *version_{};
#endif
#ifdef USE_TION_BUZZER
  switch_::Switch *buzzer_{};
#endif
#ifdef USE_TION_LED
  switch_::Switch *led_{};
#endif
#ifdef USE_TION_OUTDOOR_TEMPERATURE
  sensor::Sensor *outdoor_temperature_{};
#endif
#ifdef USE_TION_HEATER_POWER
  sensor::Sensor *heater_power_{};
#endif
#ifdef USE_TION_AIRFLOW_COUNTER
  sensor::Sensor *airflow_counter_{};
#endif
#ifdef USE_TION_PRODUCTIVITY
  sensor::Sensor *productivity_{};
#endif
#ifdef USE_TION_FILTER_TIME_LEFT
  sensor::Sensor *filter_time_left_{};
#endif
#ifdef USE_TION_FILTER_WARNOUT
  binary_sensor::BinarySensor *filter_warnout_{};
#endif
#ifdef USE_TION_RESET_FILTER
  button::Button *reset_filter_{};
  switch_::Switch *reset_filter_confirm_{};
#endif
#ifdef USE_TION_STATE_WARNOUT
  binary_sensor::BinarySensor *state_warnout_{};
  uint32_t state_timeout_{};
#endif
  uint32_t batch_timeout_{};
#ifdef USE_TION_ERRORS
  text_sensor::TextSensor *errors_{};
#endif
#ifdef USE_TION_WORK_TIME
  sensor::Sensor *work_time_{};
#endif

#ifdef TION_ENABLE_PRESETS
  number::Number *boost_time_{};
  sensor::Sensor *boost_time_left_{};
  ESPPreferenceObject boost_rtc_;
#endif

  void update_dev_info_(const dentra::tion::tion_dev_info_t &info);
};
}  // namespace tion
}  // namespace esphome
