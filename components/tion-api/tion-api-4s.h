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

#ifdef TION_ENABLE_SCHEDULER
struct tion4s_timers_state_t {
  enum { TIMERS_COUNT = 12u };
  struct {
    bool active : 8;
  } timers[TIMERS_COUNT];
};

/// Timer settings.
struct tion4s_timer_t {
  struct {
    bool monday : 1;
    bool tuesday : 1;
    bool wednesday : 1;
    bool thursday : 1;
    bool friday : 1;
    bool saturday : 1;
    bool sunday : 1;
    bool reserved : 1;
    uint8_t hours;
    uint8_t minutes;
  } schedule;
  struct {
    bool power_state : 1;
    bool sound_state : 1;
    bool led_state : 1;
    bool heater_state : 1;
    bool timer_state : 1;
    uint8_t reserved : 3;
  };
  int8_t target_temperature;
  uint8_t fan_state;
  uint8_t device_mode;
};
#endif
#pragma pack(pop)

class TionApi4s : public TionApiBase<tion4s_state_t> {
  /// Callback listener for response to request_turbo command request.
  using on_turbo_type = etl::delegate<void(const tion4s_turbo_t &turbo, const uint32_t request_id)>;
#ifdef TION_ENABLE_SCHEDULER
  /// Callback listener for response to request_time command request.
  using on_time_type = etl::delegate<void(const time_t time, uint32_t request_id)>;
  /// Callback listener for response to request_timer command request.
  using on_timer_type =
      etl::delegate<void(const uint8_t timer_id, const tion4s_timer_t &timers_state, uint32_t request_id)>;
  /// Callback listener for response to request_timers_state command request.
  using on_timers_state_type = etl::delegate<void(const tion4s_timers_state_t &timers_state, uint32_t request_id)>;
#endif
 public:
  bool read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size);

  uint16_t get_state_type() const;
  bool request_dev_status() const;
  bool request_state() const;

  bool write_state(const tion4s_state_t &state, const uint32_t request_id = 0) const;
  bool reset_filter(const tion4s_state_t &state, const uint32_t request_id = 0) const;
  bool factory_reset(const tion4s_state_t &state, const uint32_t request_id = 0) const;

#ifdef TION_ENABLE_PRESETS
  bool request_turbo() const;

  /// Callback listener for response to request_turbo command request.
  on_turbo_type on_turbo{};
  bool set_turbo(const uint16_t time, const uint32_t request_id = 0) const;
#endif

#ifdef TION_ENABLE_HEARTBEAT
  bool send_heartbeat() const;
#endif

#ifdef TION_ENABLE_SCHEDULER
  bool request_time(const uint32_t request_id = 0) const;

  /// Callback listener for response to request_time command request.
  on_time_type on_time{};
  bool set_time(const time_t time, const uint32_t request_id) const;

  bool request_timer(const uint8_t timer_id, const uint32_t request_id = 0) const;

  /// Callback listener for response to request_timer command request.
  on_timer_type on_timer{};
  /// Request all timers.
  bool request_timers(const uint32_t request_id = 0) const;

  bool request_timers_state(const uint32_t request_id = 0) const;
  /// Callback listener for response to request_timers_state command request.
  on_timers_state_type on_timers_state{};

#endif
#ifdef TION_ENABLE_DIAGNOSTIC
  bool request_errors() const;
  bool request_test() const;
#endif
};

}  // namespace tion
}  // namespace dentra
