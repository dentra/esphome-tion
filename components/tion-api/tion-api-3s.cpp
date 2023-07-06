#include <cstring>

#include "log.h"
#include "utils.h"
#include "tion-api-3s-internal.h"

namespace dentra {
namespace tion {

using namespace tion_3s;

static const char *const TAG = "tion-api-3s";

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
