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

enum {
  FRAME_MAGIC_REQ = 0x3D,
  FRAME_MAGIC_RSP = 0xB3,
  FRAME_MAGIC = 0x5A,

  FRAME_TYPE_STATE_GET_REQ = FRAME_TYPE_REQ(0x1),
  FRAME_TYPE_STATE_GET_RSP = FRAME_TYPE_RSP(0x1),
  FRAME_TYPE_STATE_SET_REQ = FRAME_TYPE_REQ(0x2),
  FRAME_TYPE_STATE_SET_RSP = FRAME_TYPE_RSP(0x2),

  FRAME_TYPE_FILTER_TIME_SET_REQ = FRAME_TYPE_REQ(0x03),
  FRAME_TYPE_FILTER_TIME_RESET_REQ = FRAME_TYPE_REQ(0x04),
  FRAME_TYPE_SRV_MODE_SET = FRAME_TYPE_REQ(0x5),
  FRAME_TYPE_HARD_RESET_REQ = FRAME_TYPE_REQ(0x6),
  FRAME_TYPE_MA_PAIRING_REQ = FRAME_TYPE_REQ(0x7),
  FRAME_TYPE_TIME_SET = FRAME_TYPE_REQ(0x8),
  FRAME_TYPE_ALARM_REQ = FRAME_TYPE_REQ(0x9),
  FRAME_TYPE_ALRAM_ON_SET_REQ = FRAME_TYPE_REQ(0xA),
  FRAME_TYPE_ALARM_OFF_SET_REQ = FRAME_TYPE_REQ(0xB),
};


#pragma pack(push, 1)

struct tion3s_frame_t {
  uint16_t type;
  uint8_t data[17];
  uint8_t magic;
};

struct tion3s_state_set_t {
  uint8_t fan_speed;
  int8_t target_temperature;
  uint8_t /*tion3s_state_t::GatePosition*/ gate_position;
  tion3s_state_t::Flags flags;
  struct {
    bool save : 1;
    bool reset : 1;
    uint8_t reserved : 6;
    uint16_t value;
  } filter_time;
  uint8_t hard_reset;
  uint8_t service_mode;
  // uint8_t reserved[7];

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

uint16_t TionApi3s::get_state_type() const { return FRAME_TYPE_STATE_GET_RSP; }

void TionApi3s::read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
  // invalid size is never possible
  // if (frame_data_size != sizeof(tion3s_state_t)) {
  //   TION_LOGW(TAG, "Incorrect state data size: %u", frame_data_size);
  //   return;
  // }

  if (frame_type == FRAME_TYPE_STATE_GET_RSP || frame_type == FRAME_TYPE_STATE_SET_RSP) {
    TION_LOGD(TAG, "Response[] State (%04X)", frame_type);
    if (this->on_state) {
      this->on_state(*static_cast<const tion3s_state_t *>(frame_data), 0);
    }
  } else {
    TION_LOGW(TAG, "Unknown frame type %04X", frame_type);
  }
}

bool TionApi3s::pair() const {
  TION_LOGD(TAG, "Request[] Pair");
  struct {
    uint8_t pair;
  } PACKED pair{.pair = 1};
  return this->write_frame(FRAME_TYPE_SRV_MODE_SET, pair);
}

bool TionApi3s::request_state() const {
  TION_LOGD(TAG, "Request[] State");
  return this->write_frame(FRAME_TYPE_STATE_GET_REQ);
}

bool TionApi3s::write_state(const tion3s_state_t &state) const {
  TION_LOGD(TAG, "Request[] Write state");
  if (!state.is_initialized()) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto mode = tion3s_state_set_t::create(state);
  return this->write_frame(FRAME_TYPE_STATE_SET_REQ, mode);
}

bool TionApi3s::reset_filter(const tion3s_state_t &state) const {
  TION_LOGD(TAG, "Request[] Reset filter");
  if (!state.is_initialized()) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto mode = tion3s_state_set_t::create(state);
  mode.filter_time.reset = true;
  return this->write_frame(FRAME_TYPE_STATE_SET_REQ, mode);
}

}  // namespace tion
}  // namespace dentra
