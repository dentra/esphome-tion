#pragma once

#include "esphome/core/component.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/number/number.h"

#include "../tion-api/log.h"
#include "../tion-api/tion-api.h"

namespace esphome {
namespace tion {

// default boost time - 10 minutes
#define DEFAULT_BOOST_TIME_SEC 10 * 60

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

  bool is_connected() const { return this->node_state == esp32_ble_tracker::ClientState::ESTABLISHED; }

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

struct tion_preset_t {
  uint8_t fan_speed;
  int8_t target_temperature;
  climate::ClimateMode mode;
};

class TionClimate : public climate::Climate {
 public:
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  virtual bool write_state() = 0;

  /**
   * Update default preset.
   * @param preset preset to update.
   * @param mode mode to update. set to climate::CLIMATE_MODE_AUTO for skip update.
   * @param fan_speed fan speed to update. set to 0 for skip update.
   * @param target_temperature target temperature to update. set to 0 for skip update.
   */
  void update_preset(climate::ClimatePreset preset, climate::ClimateMode mode, uint8_t fan_speed,
                     int8_t target_temperature) {
    if (preset > climate::CLIMATE_PRESET_NONE && preset <= climate::CLIMATE_PRESET_ACTIVITY) {
      if (mode != climate::CLIMATE_MODE_AUTO) {
        this->presets_[preset].mode = mode;
      }
      if (fan_speed != 0) {
        this->presets_[preset].fan_speed = fan_speed;
      }
      if (target_temperature != 0) {
        this->presets_[preset].target_temperature = target_temperature;
      }
      this->supported_presets_.insert(preset);
    }
  }

 protected:
  uint8_t max_fan_speed_ = 6;
  void set_fan_speed_(uint8_t fan_speed);
  uint8_t get_fan_speed_() const { return this->fan_mode_to_speed_(this->custom_fan_mode); }

  static uint8_t fan_mode_to_speed_(const optional<std::string> &fan_mode) {
    if (fan_mode.has_value()) {
      return *fan_mode.value().c_str() - '0';
    }
    return 0;
  }

  static std::string fan_speed_to_mode_(uint8_t fan_speed) {
    char fan_mode[2] = {static_cast<char>(fan_speed + '0'), 0};
    return std::string(fan_mode);
  }

  bool enable_preset_(climate::ClimatePreset preset);
  void cancel_preset_(climate::ClimatePreset preset);
  virtual bool enable_boost_() = 0;
  virtual void cancel_boost_() = 0;
  climate::ClimatePreset saved_preset_{climate::CLIMATE_PRESET_NONE};

  tion_preset_t presets_[climate::CLIMATE_PRESET_ACTIVITY + 1] = {
      {},                                                                                  // NONE, saved data
      {.fan_speed = 2, .target_temperature = 20, .mode = climate::CLIMATE_MODE_HEAT},      // HOME
      {.fan_speed = 1, .target_temperature = 10, .mode = climate::CLIMATE_MODE_FAN_ONLY},  // AWAY
      {.fan_speed = 6, .target_temperature = 10, .mode = climate::CLIMATE_MODE_FAN_ONLY},  // BOOST
      {.fan_speed = 2, .target_temperature = 23, .mode = climate::CLIMATE_MODE_HEAT},      // COMFORT
      {.fan_speed = 1, .target_temperature = 16, .mode = climate::CLIMATE_MODE_HEAT},      // ECO
      {.fan_speed = 1, .target_temperature = 18, .mode = climate::CLIMATE_MODE_HEAT},      // SLEEP
      {.fan_speed = 3, .target_temperature = 18, .mode = climate::CLIMATE_MODE_HEAT},      // ACTIVITY
  };

  std::set<climate::ClimatePreset> supported_presets_{};
};

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

  void set_state_timeout(uint32_t state_timeout) { this->state_timeout_ = state_timeout; }
  void set_persistent_connection(bool persistent_connection) { this->persistent_connection_ = persistent_connection; }

  bool is_persistent_connection() const { return this->persistent_connection_; }

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

  uint32_t state_timeout_{};
  bool persistent_connection_{};

  void read_dev_status_(const dentra::tion::tion_dev_status_t &status);
};

class TionBoostTimeNumber : public number::Number {
 public:
 protected:
  virtual void control(float value) { this->publish_state(value); }
};

class TionDisconnectMixinBase {
 protected:
  static void schedule_disconnect_(TionComponent *c, ble_client::BLEClientNode *n, uint32_t timeout);
  static void cancel_disconnect_(TionComponent *c);
};

template<typename T> class TionDisconnectMixin : private TionDisconnectMixinBase {
 public:
  void schedule_disconnect(uint32_t timeout = 3000) {
    schedule_disconnect_(static_cast<T *>(this), static_cast<T *>(this), timeout);
  }
  void cancel_disconnect() { cancel_disconnect_(static_cast<T *>(this)); }
};

class TionClimateComponent : public TionClimate, public TionComponent {};

class TionClimateComponentWithBoost : public TionClimateComponent {
 protected:
  bool enable_boost_() override;
  void cancel_boost_() override;
};

}  // namespace tion
}  // namespace esphome
