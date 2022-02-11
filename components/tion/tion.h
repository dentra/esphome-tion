#pragma once

#include "esphome/core/component.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include "../tion-api/log.h"
#include "../tion-api/tion-api.h"

namespace esphome {
namespace tion {

class TionBleNode : public ble_client::BLEClientNode {
 public:
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;
  virtual void on_ready() = 0;
  virtual void read_data(const uint8_t *data, uint16_t size) = 0;
  virtual bool write_data(const uint8_t *data, uint16_t size) const;

  virtual const esp_bt_uuid_t &get_ble_service() const = 0;
  virtual const esp_bt_uuid_t &get_ble_char_tx() const = 0;
  virtual const esp_bt_uuid_t &get_ble_char_rx() const = 0;
  virtual esp_ble_sec_act_t get_ble_encryption() const { return esp_ble_sec_act_t::ESP_BLE_SEC_ENCRYPT_MITM; }
  virtual bool ble_reg_for_notify() const { return true; }

 protected:
  uint16_t char_rx_;
  uint16_t char_tx_;
};

class TionBase : public TionBleNode {
 public:
  const esp_bt_uuid_t &get_ble_service() const override;
  const esp_bt_uuid_t &get_ble_char_tx() const override;
  const esp_bt_uuid_t &get_ble_char_rx() const override;
};

template<class T> class Tion : public TionBase, public T {
  static_assert(std::is_base_of<dentra::tion::TionApi, T>::value, "T must derived from dentra::tion::TionApi class");

 public:
  void read_data(const uint8_t *data, uint16_t size) override { T::read_data(data, size); }
  bool write_data(const uint8_t *data, uint16_t size) const override { return TionBase::write_data(data, size); }
};

class TionClimate : public climate::Climate {
 public:
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  virtual bool write_state() = 0;

 protected:
  uint8_t max_fan_speed_ = 6;
  void set_fan_mode_(uint8_t fan_speed);
  uint8_t get_fan_speed() const;
};

class TionComponent : public PollingComponent {
 public:
  void set_version(text_sensor::TextSensor *version) { this->version_ = version; }
  void set_buzzer(switch_::Switch *buzzer) { this->buzzer_ = buzzer; }
  void set_led(switch_::Switch *led) { this->led_ = led; }
  void set_temp_in(sensor::Sensor *temp_in) { this->temp_in_ = temp_in; }
  void set_temp_out(sensor::Sensor *temp_out) { this->temp_out_ = temp_out; }
  void set_heater_power(sensor::Sensor *heater_power) { this->heater_power_ = heater_power; }
  void set_airflow_counter(sensor::Sensor *airflow_counter) { this->airflow_counter_ = airflow_counter; }
  void set_filter_days_left(sensor::Sensor *filter_days_left) { this->filter_days_left_ = filter_days_left; }
  void set_filter_warnout(binary_sensor::BinarySensor *filter_warnout) { this->filter_warnout_ = filter_warnout; }

 protected:
  text_sensor::TextSensor *version_{};
  switch_::Switch *buzzer_{};
  switch_::Switch *led_{};
  sensor::Sensor *temp_in_{};
  sensor::Sensor *temp_out_{};
  sensor::Sensor *heater_power_{};
  sensor::Sensor *airflow_counter_{};
  sensor::Sensor *filter_days_left_{};
  binary_sensor::BinarySensor *filter_warnout_{};

  void read_dev_status_(const dentra::tion::tion_dev_status_t &status);
};

}  // namespace tion
}  // namespace esphome
