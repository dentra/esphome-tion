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

  FRAME_TYPE_DEV_STATUS_REQ = 0x4009,
  FRAME_TYPE_DEV_STATUS_RSP = 0x400A,

  FRAME_TYPE_AUTOKIV_PARAM_SET = 0x1240,
  FRAME_TYPE_AUTOKIV_PARAM_RSP = 0x1241,
  FRAME_TYPE_AUTOKIV_PARAM_REQ = 0x1242,
};

#pragma pack(push, 1)

// used to change state of device
struct tionlt_state_set_t {
  struct {
    bool power_state : 1;
    bool sound_state : 1;
    bool led_state : 1;
    uint8_t auto_co2 : 1;
    uint8_t heater_state : 1;
    uint8_t last_com_source : 1;  // last_com_source или save
    bool factory_reset : 1;
    bool error_reset : 1;
    bool filter_reset : 1;
    // uint8_t save;
    uint8_t reserved : 7;
  };
  uint8_t gate_position;
  int8_t target_temperature;
  uint8_t fan_speed;
  tion::tionlt_state_t::button_presets_t button_presets;
  uint16_t filter_time;
  uint8_t test_type;

  static tionlt_state_set_t create(const tion::tionlt_state_t &state) {
    tionlt_state_set_t st_set{};

    st_set.filter_time = state.counters.filter_time;

    st_set.fan_speed = state.fan_speed;
    st_set.gate_position = state.gate_position;
    st_set.target_temperature = state.target_temperature;

    st_set.power_state = state.flags.power_state;
    st_set.sound_state = state.flags.sound_state;
    st_set.led_state = state.flags.led_state;
    st_set.heater_state = state.flags.heater_state;

    st_set.test_type = state.test_type;
    st_set.button_presets = state.button_presets;

    return st_set;
  }
};

#pragma pack(pop)

}  // namespace tion_lt
}  // namespace dentra
