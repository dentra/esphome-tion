#pragma once

#include "esphome/core/component.h"

#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/number/number.h"

#include "../vport/vport_component.h"

#include "../tion-api/log.h"
#include "../tion-api/tion-api.h"

namespace esphome {
namespace tion {

#ifdef TION_ENABLE_PRESETS
// default boost time - 10 minutes
#define DEFAULT_BOOST_TIME_SEC 10 * 60

struct tion_preset_t {
  uint8_t fan_speed;
  int8_t target_temperature;
  climate::ClimateMode mode;
};
#endif

class TionClimate : public climate::Climate {
 public:
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  virtual bool write_climate_state() = 0;
#ifdef TION_ENABLE_PRESETS
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
#endif
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
#ifdef TION_ENABLE_PRESETS
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
#endif
};

class TionComponent : public Component {
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

enum TionVPortType : uint8_t { VPORT_BLE, VPORT_UART, VPORT_TCP };

class TionClimateComponentBase : public TionClimate, public TionComponent {
 public:
  void dump_settings(const char *TAG, const char *component) const;
#ifdef TION_ENABLE_HEARTBEAT
  virtual bool send_heartbeat() const = 0;
#endif

  void set_vport_type(TionVPortType vport_type) { this->vport_type_ = vport_type; }

 protected:
  enum : uint8_t {
    DIRTY_STATE = 1 << 0,
  };
  uint8_t dirty_flag_{};
  bool is_dirty_(uint8_t flag) { return (this->dirty_flag_ & flag) != 0; }
  void set_dirty_(uint8_t flag) { this->dirty_flag_ |= flag; }
  void drop_dirty_(uint8_t flag) { this->dirty_flag_ &= ~flag; }
#ifdef TION_ENABLE_PRESETS
  bool enable_boost_() override;
  void cancel_boost_() override;
#endif

  TionVPortType vport_type_{};
};

/**
 * @param tion_api_type TionApi implementation.
 * @param tion_state_type Tion state struct.
 */
template<class tion_api_type, class tion_state_type> class TionClimateComponent : public TionClimateComponentBase {
  static_assert(std::is_base_of<dentra::tion::TionApiBase<tion_state_type>, tion_api_type>::value,
                "tion_api_type is not derived from TionApiBase");

  using this_type = TionClimateComponent<tion_api_type, tion_state_type>;

 public:
  explicit TionClimateComponent(tion_api_type *api, vport::VPortComponent<uint16_t> *vport) : api_(api), vport_(vport) {
    vport->on_ready.set<tion_api_type, &tion_api_type::request_dev_status>(*api);
    vport->on_update.set<tion_api_type, &tion_api_type::request_state>(*api);
    vport->on_frame.set<tion_api_type, &tion_api_type::read_frame>(*api);

    this->api_->on_dev_status.template set<this_type, &this_type::on_dev_status>(*this);
    this->api_->on_state.template set<this_type, &this_type::on_state>(*this);
  }

  bool write_climate_state() override {
    this->publish_state();
    this->set_dirty_(DIRTY_STATE);
    this->vport_->update();
    return true;
  }

  virtual void update_state(const tion_state_type &state) = 0;
  virtual void dump_state(const tion_state_type &state) const = 0;
  virtual void flush_state(const tion_state_type &state) const = 0;

  void on_state(const tion_state_type &state, const uint32_t request_id) {
    if (this->is_dirty_(DIRTY_STATE)) {
      this->drop_dirty_(DIRTY_STATE);
      this->flush_state(state);
      return;
    }
    this->update_state(state);
#if TION_LOG_LEVEL >= TION_LOG_LEVEL_VERBOSE
    this->dump_state(state);
#endif
  }

  void on_dev_status(const dentra::tion::tion_dev_status_t &status) { this->update_dev_status_(status); }

#ifdef TION_ENABLE_HEARTBEAT
  bool send_heartbeat() const { return this->api_->send_heartbeat(); }
#endif

 protected:
  tion_api_type *api_;
  vport::VPortComponent<uint16_t> *vport_;
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

}  // namespace tion
}  // namespace esphome
