#include <cstring>

#include "log.h"
#include "utils.h"
#include "tion-api-3s.h"

namespace dentra {
namespace tion {

static const char *const TAG = "tion-api-3s";

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
  tion3s_state_t::Flags flags;                             // 3-4
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
  static tion3s_state_set_t create(const tion3s_state_t &state) {
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

uint16_t TionApi3s::get_state_type() const { return FRAME_TYPE_RSP(FRAME_TYPE_STATE_GET); }

void TionApi3s::read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
  // invalid size is never possible
  // if (frame_data_size != sizeof(tion3s_state_t)) {
  //   TION_LOGW(TAG, "Incorrect state data size: %u", frame_data_size);
  //   return;
  // }

  if (frame_type == FRAME_TYPE_RSP(FRAME_TYPE_STATE_GET)) {
    TION_LOGD(TAG, "Response[] State Get (%04X)", frame_type);
    if (this->on_state) {
      this->on_state(*static_cast<const tion3s_state_t *>(frame_data), 0);
    }
  } else if (frame_type == FRAME_TYPE_RSP(FRAME_TYPE_STATE_SET)) {
    TION_LOGD(TAG, "Response[] State Set (%04X)", frame_type);
    if (this->on_state) {
      this->on_state(*static_cast<const tion3s_state_t *>(frame_data), 0);
    }
  } else if (frame_type == FRAME_TYPE_RSP(FRAME_TYPE_FILTER_TIME_RESET)) {
    TION_LOGD(TAG, "Response[] Command 4 (%04X): %s", frame_type, hexencode(frame_data, frame_data_size).c_str());
    // ответ такого вида, к сожалению, значения пока не понятны:
    // на прошивке 003C
    // B3 40 11 00 08 00 08 00 08 00 00 00 00 00 00 00 00 00 00 5A
    // на прошивке 0033, после команды 0x1
    // 3D 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 5A
  } else {
    TION_LOGW(TAG, "Response[] Unknown (%04X): %s", frame_type, hexencode(frame_data, frame_data_size).c_str());
  }
}

bool TionApi3s::pair() const {
  TION_LOGD(TAG, "Request[] Pair");
  struct {
    uint8_t pair;
  } PACKED pair{.pair = 1};
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_SRV_MODE_SET), pair);
}

bool TionApi3s::request_state() const {
  TION_LOGD(TAG, "Request[] State Get");
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_STATE_GET));
}

bool TionApi3s::write_state(const tion3s_state_t &state) const {
  TION_LOGD(TAG, "Request[] State Set");
  if (!state.is_initialized()) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto mode = tion3s_state_set_t::create(state);
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_STATE_SET), mode);
}

bool TionApi3s::reset_filter(const tion3s_state_t &state) const {
  TION_LOGD(TAG, "Request[] Filter Time Reset");
  // return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_FILTER_TIME_RESET));

  // предположительно сброс ресурса фильтра выполняется командой 2
  // 3D:01:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:5A
  // 3D:01:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:5A
  // 3D:02:01:17:02:0A:01:02:00:00:00:00:00:00:00:00:00:00:00:5A
  // 3D:01:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:5A
  // 3D:04:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:5A
  // 3D:04:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:5A
  // 3D:04:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:5A

  // еще пример
  // 3D:02:01:17:02:0A:01:02:00:00:00:00:00:00:00:00:00:00:00:5A
  // 3D:01:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:5A
  // 3D:04:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:5A

  if (!state.is_initialized()) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }

  auto set = tion3s_state_set_t::create(state);
  set.flags.preset_state = true;
  set.filter_time.reset = true;
  set.filter_time.value = 0;
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_STATE_SET), set);
}

bool TionApi3s::request_command4() const {
  TION_LOGD(TAG, "Request[] Command 4");
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_FILTER_TIME_RESET));
}

}  // namespace tion
}  // namespace dentra
