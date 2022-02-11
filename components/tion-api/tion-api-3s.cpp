#include <cstring>

#include "log.h"
#include "utils.h"
#include "tion-api-3s.h"

namespace dentra {
namespace tion {

static const char *const TAG = "tion-api-3s";

enum {
  FRAME_MAGIC = 0x5A,

  FRAME_TYPE_STATE_REQ = 0x013D,
  FRAME_TYPE_STATE_RSP = 0x10B3,
  FRAME_TYPE_STATE_SET = 0x023D,
  // FRAME_TYPE_FILTER_TIME_SET = 0x033D,
  // FRAME_TYPE_FILTER_TIME_RESET = 0x043D,
  FRAME_TYPE_SRV_MODE_SET = 0x053D,
  // FRAME_TYPE_HARD_RESET_REQ = 0x063D,
  // FRAME_TYPE_MA_PAIRING_REQ = 0x073D,
  // FRAME_TYPE_TIME_SET = 0x083D,
  // FRAME_TYPE_ALARM_REQ = 0x093D,
  // FRAME_TYPE_ALRAM_ON_SET = 0x0A3D,
  // FRAME_TYPE_ALARM_OFF_SET = 0x0B3D,
};

#pragma pack(push, 1)

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

struct tion3s_frame_t {
  uint16_t type;
  uint8_t data[17];
  uint8_t magic;
};

#pragma pack(pop)

bool TionsApi3s::read_data(const uint8_t *data, uint16_t size) {
  TION_LOGV(TAG, "Read data: %s", hexencode(data, size).c_str());
  if (data == nullptr || size == 0) {
    TION_LOGW(TAG, "Empy frame data");
    return false;
  }
  if (size != sizeof(tion3s_frame_t)) {
    TION_LOGW(TAG, "Invalid frame size %u", size);
    return false;
  }
  auto frame = reinterpret_cast<const tion3s_frame_t *>(data);
  if (frame->magic != FRAME_MAGIC) {
    TION_LOGW(TAG, "Invalid frame crc %02X", data[size - 1]);
    return false;
  }

  if (frame->type == FRAME_TYPE_STATE_RSP) {
    const void *frame_data = frame->data;
    this->read(*static_cast<const tion3s_state_t *>(frame_data));
    return true;
  }

  TION_LOGW(TAG, "Unknown frame type %04X", frame->type);

  return false;
}

bool TionsApi3s::write_frame_(uint16_t frame_type, const void *frame_data, uint16_t frame_data_size) const {
  tion3s_frame_t frame{.type = frame_type, .data = {}, .magic = FRAME_MAGIC};
  if (frame_data_size <= sizeof(frame.data)) {
    std::memcpy(frame.data, frame_data, frame_data_size);
  }
  return this->write_data(reinterpret_cast<const uint8_t *>(&frame), sizeof(frame));
}

bool TionsApi3s::pair() const {
  TION_LOGD(TAG, "Enter pair mode");
  struct {
    uint8_t pair;
  } PACKED pair{.pair = 1};
  return this->write_frame_(FRAME_TYPE_SRV_MODE_SET, &pair, sizeof(pair));
}

bool TionsApi3s::request_state() const {
  TION_LOGD(TAG, "Request state");
  return this->write_(FRAME_TYPE_STATE_REQ);
}

bool TionsApi3s::write_state(const tion3s_state_t &state) const {
  TION_LOGD(TAG, "Write state");
  if (state.firmware_version == 0) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto mode = tion3s_state_set_t::create(state);
  return this->write_frame_(FRAME_TYPE_STATE_SET, &mode, sizeof(mode));
}

bool TionsApi3s::reset_filter(const tion3s_state_t &state) const {
  TION_LOGD(TAG, "Reset filter");
  if (state.firmware_version == 0) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto mode = tion3s_state_set_t::create(state);
  mode.filter_time.reset = true;
  return this->write_frame_(FRAME_TYPE_STATE_SET, &mode, sizeof(mode));
}

}  // namespace tion
}  // namespace dentra
