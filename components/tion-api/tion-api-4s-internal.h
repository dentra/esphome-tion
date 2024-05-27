#pragma once

#include <cstdint>

#include "tion-api-internal.h"

namespace dentra {
namespace tion_4s {

enum {
  FRAME_TYPE_STATE_SET = 0x3230,  // no save req
  FRAME_TYPE_STATE_RSP = 0x3231,
  FRAME_TYPE_STATE_REQ = 0x3232,
  FRAME_TYPE_STATE_SAV = 0x3234,  // save req

  FRAME_TYPE_DEV_INFO_REQ = 0x3332,
  FRAME_TYPE_DEV_INFO_RSP = 0x3331,

  FRAME_TYPE_TEST_REQ = 0x3132,
  FRAME_TYPE_TEST_RSP = 0x3131,  // returns 440 bytes struct

  FRAME_TYPE_TIMER_SET = 0x3430,
  FRAME_TYPE_TIMER_REQ = 0x3432,
  FRAME_TYPE_TIMER_RSP = 0x3431,

  FRAME_TYPE_TIMERS_STATE_REQ = 0x3532,
  FRAME_TYPE_TIMERS_STATE_RSP = 0x3531,

  FRAME_TYPE_TIME_SET = 0x3630,
  FRAME_TYPE_TIME_REQ = 0x3632,
  FRAME_TYPE_TIME_RSP = 0x3631,

  FRAME_TYPE_ERR_CNT_REQ = 0x3732,
  FRAME_TYPE_ERR_CNT_RSP = 0x3731,

  FRAME_TYPE_CURR_TEST_SET = 0x3830,  // BLE Only
  FRAME_TYPE_CURR_TEST_REQ = 0x3832,  // BLE Only
  FRAME_TYPE_CURR_TEST_RSP = 0x3831,  // BLE Only

  FRAME_TYPE_TURBO_SET = 0x4130,  // BLE Only
  FRAME_TYPE_TURBO_REQ = 0x4132,  // BLE Only
  FRAME_TYPE_TURBO_RSP = 0x4131,  // BLE Only

  FRAME_TYPE_HEARTBIT_REQ = 0x3932,  // UART Only, every 3 sec
  FRAME_TYPE_HEARTBIT_RSP = 0x3931,  // UART Only
};

#pragma pack(push, 1)
// NOLINTNEXTLINE(readability-identifier-naming)
struct tion4s_state_t {
  enum { ERROR_MIN_BIT = 0, ERROR_MAX_BIT = 10, WARNING_MIN_BIT = 24, WARNING_MAX_BIT = 29, GATE_ERROR_BIT = 1 << 0 };

  enum HeaterMode : uint8_t {
    HEATER_MODE_HEATING = 0,
    HEATER_MODE_FANONLY = 1,
  };
  // air intake: indoor, outdoor, mixed
  enum GatePosition : uint8_t {
    // приток
    GATE_POSITION_OUTDOOR = 0,
    // рециркуляция
    GATE_POSITION_INDOOR = 1,
  };

  enum HeaterPresent : uint16_t {
    HEATER_PRESENT_NONE = 0,
    HEATER_PRESENT_1000W = 1,
    HEATER_PRESENT_1400W = 2,
  };

  // Байт 0-1.
  struct {
    // Байт 0, бит 0. Состояние (power state).
    bool power_state : 1;
    // Байт 0, бит 1. Состояние звуковых оповещений.
    bool sound_state : 1;
    // Байт 0, бит 2. Состояние световых оповещений.
    bool led_state : 1;
    // Байт 0, бит 3. Сотояние обогревателя.
    bool heater_state : 1;
    // Байт 0, бит 4. Режим обогрева.
    HeaterMode heater_mode : 1;
    // Байт 0, бит 5.
    tion::CommSource comm_source : 1;
    // Байт 0, бит 6. Предупреждение о необходимости замены фильтра.
    bool filter_state : 1;
    // Байт 0, бит 7, Байт 1, бит 0-1. Мощность тэна: 0 - 0 kW, 1 - 1 kW, 2 - 1.4 kW.
    HeaterPresent heater_present : 3;
    // Байт 1, бит 2. Состояния подключения MagicAir.
    bool ma_connected : 1;
    // Байт 1, бит 3. MagicAir auto control.
    bool ma_auto : 1;
    // Байт 1, бит 4.
    bool active_timer : 1;
    // зарезервированно.
    uint8_t reserved : 3;
  };
  // Байт 2. settings: gate position (0 - inflow, 1 - recirculation).
  GatePosition gate_position;
  // Байт 3. settings: target temperature.
  int8_t target_temperature;
  // Байт 4. settings: fan speed 1-6.
  uint8_t fan_speed;
  // Байт 5. sensor: outdoor temperature.
  int8_t outdoor_temperature;
  // Байт 6. sensor: current temperature.
  int8_t current_temperature;
  // Байт 7. sensor: ctrl pcb temperature.
  int8_t pcb_ctl_temperature;
  // Байт 8. sensor: pwr pcb temperature.
  int8_t pcb_pwr_temperature;
  // Байт 9-24. 9-12 - work_time, 13-16 - fan_time, 17-20 - filter_time, 21-24 - airflow_counter
  tion::tion_state_counters_t<15> counters;
  // Байт 25-28. Ошибки и/или предупредждения.
  uint32_t errors;
  // fan speed limit.
  uint8_t max_fan_speed;
  // Heater power in %.
  uint8_t heater_var;

  static std::string decode_errors(uint32_t errors) {
    return tion::decode_errors(errors, ERROR_MIN_BIT, ERROR_MAX_BIT, WARNING_MIN_BIT, WARNING_MAX_BIT);
  }
};

// NOLINTNEXTLINE(readability-identifier-naming)
struct tion4s_turbo_t {
  uint8_t is_active;
  uint16_t turbo_time;  // time in seconds
  uint8_t err_code;
};

#ifdef TION_ENABLE_SCHEDULER
// NOLINTNEXTLINE(readability-identifier-naming)
struct tion4s_timers_state_t {
  enum { TIMERS_COUNT = 12u };
  struct {
    bool active : 8;
  } timers[TIMERS_COUNT];
};

/// Timer settings. Used for get and set operations.
// NOLINTNEXTLINE(readability-identifier-naming)
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
    tion4s_state_t::HeaterMode heater_mode : 1;
    // on/off timer.
    bool timer_state : 1;
    uint8_t reserved : 3;
  };
  int8_t target_temperature;
  uint8_t fan_state;
  uint8_t device_mode;
};
#endif

// структура для изменения состояния
// NOLINTNEXTLINE(readability-identifier-naming)
struct tion4s_state_set_t {
  // Байт 0-1.
  struct {
    // Байт 0, бит 0. состояние (power state)
    bool power_state : 1;
    // Байт 0, бит 1. состояние звуковых оповещений
    bool sound_state : 1;
    // Байт 0, бит 2. состояние световых оповещений
    bool led_state : 1;
    // Байт 0, бит 3.
    tion4s_state_t::HeaterMode heater_mode : 1;
    // Байт 0, бит 4.
    tion::CommSource comm_source : 1;
    // Байт 0, бит 5.
    bool factory_reset : 1;
    // Байт 0, бит 6.
    bool error_reset : 1;
    // Байт 0, бит 7.
    bool filter_reset : 1;
    // Байт 1, бит 0.
    bool ma_connected : 1;
    // Байт 1, бит 1.
    bool ma_auto : 1;
    uint8_t reserved : 6;
  };
  // Байт 2.
  tion4s_state_t::GatePosition gate_position;
  // Байт 3. температрура нагревателя.
  int8_t target_temperature;
  // Байт 4. Скорость вентиляции.
  uint8_t fan_speed;
  // Байт 5-6. filter time in days
  // TODO синхронизировал с tion remote. работало с uint32_t, посмотреть в прошивке.
  uint16_t filter_time;

  static tion4s_state_set_t create(const tion::TionState &state) {
    tion4s_state_set_t st_set{};

    st_set.power_state = state.power_state;
    st_set.sound_state = state.sound_state;
    st_set.led_state = state.led_state;
    st_set.heater_mode = state.heater_state                          //-//
                             ? tion4s_state_t::HEATER_MODE_HEATING   //-//
                             : tion4s_state_t::HEATER_MODE_FANONLY;  //-//
    st_set.comm_source = state.comm_source;
    st_set.ma_auto = state.auto_state;
    st_set.ma_connected = state.auto_state;

    st_set.gate_position = state.gate_position != tion::TionGatePosition::OUTDOOR  //-//
                               ? tion4s_state_t::GATE_POSITION_INDOOR              //-//
                               : tion4s_state_t::GATE_POSITION_OUTDOOR;            //-//
    st_set.target_temperature = state.target_temperature;

    st_set.fan_speed = state.fan_speed == 0 ? static_cast<uint8_t>(1) : state.fan_speed;
    // st_set.filter_time = state.counters.filter_time
    return st_set;
  }
};

#ifdef TION_ENABLE_SCHEDULER
// NOLINTNEXTLINE(readability-identifier-naming)
struct tion4s_time_t {
  int64_t unix_time;
};
#endif
#ifdef TION_ENABLE_DIAGNOSTIC
// NOLINTNEXTLINE(readability-identifier-naming)
struct tion4s_errors_t {
  enum { ERROR_TYPES_COUNT = 32u };
  uint8_t er[ERROR_TYPES_COUNT];
};
#endif

#pragma pack(pop)

}  // namespace tion_4s
}  // namespace dentra
