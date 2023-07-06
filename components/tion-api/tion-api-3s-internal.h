#pragma once

#include <cstdint>

#include "tion-api-3s.h"

namespace dentra {
namespace tion_3s {

#define FRAME_TYPE(typ, cmd) ((cmd << 8) | (typ & 0xFF))
#define FRAME_TYPE_REQ(cmd) FRAME_TYPE(FRAME_MAGIC_REQ, cmd)
#define FRAME_TYPE_RSP(cmd) FRAME_TYPE(FRAME_MAGIC_RSP, (cmd << 4))

enum : uint8_t {
  // request state
  FRAME_TYPE_STATE_GET = 0x1,
  // write state
  FRAME_TYPE_STATE_SET = 0x2,
  // ???
  FRAME_TYPE_FILTER_TIME_SET = 0x3,
  // ??? reset filter
  FRAME_TYPE_FILTER_TIME_RESET = 0x4,
  // send sevice mode flags
  FRAME_TYPE_SRV_MODE_SET = 0x5,
  FRAME_TYPE_HARD_RESET = 0x6,
  FRAME_TYPE_MA_PAIRING = 0x7,
  // set time
  FRAME_TYPE_TIME_SET = 0x8,
  // get alarm
  FRAME_TYPE_ALARM = 0x9,
  // set alrm to on
  FRAME_TYPE_ALRAM_ON = 0xA,
  // set alrm to off
  FRAME_TYPE_ALARM_OFF = 0xB,
};

#pragma pack(push, 1)

struct tion3s_frame_t {
  uint16_t type;
  uint8_t data[17];
  uint8_t magic;
};

struct tion3s_state_set_t {
  uint8_t fan_speed;                                       // 0
  int8_t target_temperature;                               // 1
  uint8_t /*tion3s_state_t::GatePosition*/ gate_position;  // 2
  tion::tion3s_state_t::Flags flags;                       // 3-4
  struct {
    bool save : 1;
    bool reset : 1;
    uint8_t reserved : 6;
    uint16_t value;
  } filter_time;         // 5-7
  uint8_t hard_reset;    // 8
  uint8_t service_mode;  // 9
  // uint8_t reserved[7];

  //        0  1  2  3  4  5  6  7  8  9
  // 3D:02 01 17 02 0A.01 02.00.00 00 00 00:00:00:00:00:00:00:5A
  static tion3s_state_set_t create(const tion::tion3s_state_t &state) {
    tion3s_state_set_t st_set{};

    st_set.fan_speed = state.fan_speed;
    st_set.target_temperature = state.target_temperature;
    st_set.gate_position = state.gate_position;
    st_set.flags = state.flags;
    st_set.filter_time.value = state.filter_time;

    return st_set;
  }
};

#pragma pack(pop)

}  // namespace tion_3s
}  // namespace dentra
