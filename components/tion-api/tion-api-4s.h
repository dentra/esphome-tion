#pragma once

#include "tion-api.h"

namespace dentra {
namespace tion {

#pragma pack(push, 1)
struct tion4s_state_t {
  enum HeaterMode : uint8_t {
    HEATER_MODE_HEATING = 0,
    HEATER_MODE_TEMPERATURE_MAINTENANCE = 1,
  };
  // air intake: indoor, outdoor, mixed
  enum GatePosition : uint8_t {
    // приток
    GATE_POSITION_INFLOW = 0,
    // рециркуляция
    GATE_POSITION_RECIRCULATION = 1,
  };

  enum HeaterPresent : uint16_t {
    HEATER_PRESENT_NONE = 0,
    HEATER_PRESENT_1000W = 1,
    HEATER_PRESENT_1400W = 2,
  };

  struct {
    // состояние (power state)
    bool power_state : 1;
    // состояние звуковых оповещений
    bool sound_state : 1;
    // состояние световых оповещений
    bool led_state : 1;
    // сотояние обогрева
    bool heater_state : 1;
    // режим обогрева
    uint8_t /*HeaterMode*/ heater_mode : 1;
    //
    uint8_t last_com_source : 1;
    // предупреждение о необходимости замены фильтра
    bool filter_wornout : 1;
    // мощность тэна: 0 - 0 kW, 1 - 1 kW, 2 - 1.4 kW
    uint8_t /*HeaterPresent*/ heater_present : 3;
    // MagicAir control
    bool ma : 1;
    // MagicAir auto control
    bool ma_auto : 1;
    //
    bool active_timer : 1;
    // зарезервированно
    uint8_t reserved : 3;
  } flags;
  GatePosition gate_position;
  // теператрура нагрева
  int8_t target_temperature;
  // скорость вентилятора 0-5
  uint8_t fan_speed;
  int8_t current_temperature;
  int8_t outdoor_temperature;
  int8_t pcb_ctl_temperature;
  int8_t pcb_pwr_temperature;
  tion_state_counters_t counters;
  uint32_t errors;
  // fan speed limit
  uint8_t max_fan_speed;
  // Heater power in %
  // use heater_power() to get actual consumption in W
  uint8_t heater_var;

  float heater_power() const;
};

struct tion4s_turbo_t {
  uint8_t is_active;
  uint16_t turbo_time;  // time in seconds
  uint8_t err_code;
};

struct tion4s_time_t {
  int64_t unix_time;
  time_t get_time() const { return unix_time / 1000000; }
};

#pragma pack(pop)

class TionApi4s : public TionApi {
 public:
  virtual void read(const tion_dev_status_t &status) {}
  virtual void read(const tion4s_state_t &state) {}
  virtual void read(const tion4s_turbo_t &turbo) {}
  virtual void read(const tion4s_time_t &time) {}

  void request_dev_status() override;
  void request_state() override;

  void request_turbo();
  void request_time();
  void request_errors();
  void request_test();
  void request_timers();

  bool write_state(const tion4s_state_t &state) const;
  bool reset_filter(const tion4s_state_t &state) const;
  bool factory_reset(const tion4s_state_t &state) const;
  bool set_turbo_time(uint16_t time) const;
  bool set_time(int64_t time) const;

 protected:
  void read_(uint16_t frame_type, const void *frame_data, uint16_t frame_data_size) override;
};

}  // namespace tion
}  // namespace dentra
