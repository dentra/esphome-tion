#pragma once

#include <cstdint>

#include "tion-api-4s.h"

namespace dentra {
namespace tion_4s {

enum {
  FRAME_TYPE_STATE_SET = 0x3230,  // no save req
  FRAME_TYPE_STATE_RSP = 0x3231,
  FRAME_TYPE_STATE_REQ = 0x3232,
  FRAME_TYPE_STATE_SAV = 0x3234,  // save req

  FRAME_TYPE_DEV_STATUS_REQ = 0x3332,
  FRAME_TYPE_DEV_STATUS_RSP = 0x3331,

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

  FRAME_TYPE_CURR_TEST_SET = 0x3830,
  FRAME_TYPE_CURR_TEST_REQ = 0x3832,
  FRAME_TYPE_CURR_TEST_RSP = 0x3831,

  FRAME_TYPE_TURBO_SET = 0x4130,  // BLE Only
  FRAME_TYPE_TURBO_REQ = 0x4132,  // BLE Only
  FRAME_TYPE_TURBO_RSP = 0x4131,  // BLE Only

  FRAME_TYPE_HEARTBIT_REQ = 0x3932,  // every 3 sec
  FRAME_TYPE_HEARTBIT_RSP = 0x3931,
};

#pragma pack(push, 1)

// структура для изменения состояния
struct tion4s_state_set_t {
  struct {
    // состояние (power state)
    bool power_state : 1;
    // состояние звуковых оповещений
    bool sound_state : 1;
    // состояние световых оповещений
    bool led_state : 1;
    uint8_t /*HeaterMode*/ heater_mode : 1;
    bool last_com_source : 1;
    bool factory_reset : 1;
    bool error_reset : 1;
    bool filter_reset : 1;
    bool ma : 1;
    bool ma_auto : 1;
    uint8_t reserved : 6;
  };
  tion::tion4s_state_t::GatePosition gate_position;
  // температрура нагревателя.
  int8_t target_temperature;
  uint8_t fan_speed;
  // filter time in days
  // TODO возможно должен/может быть uint16_t
  uint32_t filter_time;

  static tion4s_state_set_t create(const tion::tion4s_state_t &state) {
    tion4s_state_set_t st_set{};

    st_set.power_state = state.flags.power_state;
    st_set.heater_mode = state.flags.heater_mode;
    st_set.gate_position = state.gate_position;
    st_set.target_temperature = state.target_temperature;
    st_set.led_state = state.flags.led_state;
    st_set.sound_state = state.flags.sound_state;
    st_set.fan_speed = state.fan_speed;
    st_set.filter_time = state.counters.filter_time;
    st_set.ma_auto = state.flags.ma_auto;
    st_set.ma = state.flags.ma;

    return st_set;
  }
};

#ifdef TION_ENABLE_SCHEDULER
struct tion4s_time_t {
  int64_t unix_time;
};
#endif
#ifdef TION_ENABLE_DIAGNOSTIC
struct tion4s_errors_t {
  enum { ERROR_TYPES_COUNT = 32u };
  uint8_t er[ERROR_TYPES_COUNT];
};
#endif

#pragma pack(pop)

}  // namespace tion4s
}  // namespace dentra
