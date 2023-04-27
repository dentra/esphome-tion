#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/number/number.h"

#include "../tion-api/tion-api.h"

namespace esphome {
namespace tion {
class TionComponent : public PollingComponent {
 public:
  void setup() override;

  void set_version(text_sensor::TextSensor *version) { this->version_ = version; }
  void set_buzzer(switch_::Switch *buzzer) { this->buzzer_ = buzzer; }
  void set_led(switch_::Switch *led) { this->led_ = led; }
  void set_outdoor_temperature(sensor::Sensor *outdoor_temperature) {
    this->outdoor_temperature_ = outdoor_temperature;
  }
  void set_heater_power(sensor::Sensor *heater_power) { this->heater_power_ = heater_power; }
  void set_airflow_counter(sensor::Sensor *airflow_counter) { this->airflow_counter_ = airflow_counter; }
  void set_filter_time_left(sensor::Sensor *filter_time_left) { this->filter_time_left_ = filter_time_left; }
  void set_filter_warnout(binary_sensor::BinarySensor *filter_warnout) { this->filter_warnout_ = filter_warnout; }

  void set_boost_time(number::Number *boost_time) { this->boost_time_ = boost_time; }
  void set_boost_time_left(sensor::Sensor *boost_time_left) { this->boost_time_left_ = boost_time_left; }

 protected:
  text_sensor::TextSensor *version_{};
  switch_::Switch *buzzer_{};
  switch_::Switch *led_{};
  sensor::Sensor *outdoor_temperature_{};
  sensor::Sensor *heater_power_{};
  sensor::Sensor *airflow_counter_{};
  sensor::Sensor *filter_time_left_{};
  binary_sensor::BinarySensor *filter_warnout_{};

  number::Number *boost_time_{};
  sensor::Sensor *boost_time_left_{};

#ifdef TION_ENABLE_PRESETS
  ESPPreferenceObject rtc_;
#endif

  void update_dev_status_(const dentra::tion::tion_dev_status_t &status);
};
}  // namespace tion
}  // namespace esphome
