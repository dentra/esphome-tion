#pragma once

#include <cstdint>

#include "tion-api-lt.h"

namespace dentra {
namespace tion_lt {

enum {
  FRAME_TYPE_STATE_SET = 0x1230,  // set no save req
  FRAME_TYPE_STATE_RSP = 0x1231,
  FRAME_TYPE_STATE_REQ = 0x1232,
  FRAME_TYPE_STATE_SAV = 0x1234,  // set save req

  FRAME_TYPE_DEV_INFO_REQ = 0x4009,
  FRAME_TYPE_DEV_INFO_RSP = 0x400A,

  FRAME_TYPE_AUTOKIV_PARAM_SET = 0x1240,
  FRAME_TYPE_AUTOKIV_PARAM_RSP = 0x1241,
  FRAME_TYPE_AUTOKIV_PARAM_REQ = 0x1242,

  FRAME_TYPE_TEST_REQ = 0x1111,
  FRAME_TYPE_TEST_RSP = 0x2222,  // returns 0x400 bytes
};

#pragma pack(push, 1)

// used to change state of device
// NOLINTNEXTLINE(readability-identifier-naming)
struct tionlt_state_set_t {
  struct {
    // Байт 0, бит 0
    bool power_state : 1;
    // Байт 0, бит 1
    bool sound_state : 1;
    // Байт 0, бит 2
    bool led_state : 1;
    // Байт 0, бит 3
    bool ma_auto : 1;
    // Байт 0, бит 4
    bool heater_state : 1;
    // Байт 0, бит 5
    tion::CommSource comm_source : 1;  // last_com_source или save
    // Байт 0, бит 6
    bool factory_reset : 1;
    // Байт 0, бит 7
    bool error_reset : 1;
    // Байт 1, бит 0
    bool filter_reset : 1;
    // uint8_t save;
    uint8_t reserved : 7;
  };
  // Байт 2
  tion::tionlt_state_t::GateState gate_state;
  // Байт 3
  int8_t target_temperature;
  // Байт 4. Скорость вентиляции.
  uint8_t fan_speed;
  tion::tionlt_state_t::button_presets_t button_presets;
  uint16_t filter_time;
  uint8_t test_type;

  static tionlt_state_set_t create(const tion::tionlt_state_t &state) {
    tionlt_state_set_t st_set{};

    st_set.fan_speed = state.fan_speed == 0 ? 1 : state.fan_speed;
    st_set.target_temperature = state.target_temperature;

    // // FIXME в tion remote выставляется так:
    // //   (fan_speed > 0 || target_temperature > 0) ? OPENED : CLOSED
    // // т.е. с учетом того что fan_speed всегда > 0, то всегда OPENED
    st_set.gate_state = state.gate_state;

    st_set.power_state = state.flags.power_state;
    st_set.sound_state = state.flags.sound_state;
    st_set.led_state = state.flags.led_state;
    st_set.heater_state = state.flags.heater_state;

    st_set.button_presets = state.button_presets;

    st_set.ma_auto = state.flags.ma_auto;
    st_set.comm_source = st_set.ma_auto ? tion::CommSource::AUTO : tion::CommSource::USER;

    return st_set;
  }
};

#pragma pack(pop)

}  // namespace tion_lt
}  // namespace dentra
