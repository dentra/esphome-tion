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
    bool filter_warnout : 1;
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
  int8_t outdoor_temperature;
  int8_t current_temperature;
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

#pragma pack(pop)

class TionApi4s : public TionApi<tion4s_state_t> {
 public:
  explicit TionApi4s(TionFrameWriter *writer) : TionApi(writer) {}

  uint16_t get_state_type() const override;

  virtual void on_turbo(const tion4s_turbo_t &turbo, const uint32_t request_id) {}
  virtual void on_time(const time_t time, const uint32_t request_id) {}

  bool request_dev_status() const override;
  bool request_state() const override;

  bool request_turbo() const;
  bool request_time(const uint32_t request_id = 0) const;
  bool request_errors() const;
  bool request_test() const;
  bool request_timer(const uint8_t timer_id, const uint32_t request_id = 0) const;
  bool request_timers(const uint32_t request_id = 0) const;

  bool write_state(const tion4s_state_t &state, const uint32_t request_id = 0) const;
  bool reset_filter(const tion4s_state_t &state, const uint32_t request_id = 0) const;
  bool factory_reset(const tion4s_state_t &state, const uint32_t request_id = 0) const;
  bool set_turbo_time(const uint16_t time, const uint32_t request_id = 0) const;
  bool set_time(const time_t time, const uint32_t request_id) const;

  bool send_heartbeat() const override;

 protected:
  bool read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) override;
};

}  // namespace tion
}  // namespace dentra
