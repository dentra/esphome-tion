#include <cstring>
#include <cinttypes>

#include "log.h"
#include "utils.h"
#include "tion-api-3s-internal.h"

namespace dentra {
namespace tion {

using namespace tion_3s;

static const char *const TAG = "tion-api-3s";

void tion3s_state_t::for_each_error(const std::function<void(uint8_t error, const char type[3])> &fn) const {
  if (this->last_error != 0) {
    fn(this->last_error, "EC");
  }
}

struct Tion3sTimersResponse {
  struct {
    uint8_t hours;
    uint8_t minutes;
  } timers[4];
};

uint16_t Tion3sApi::get_state_type() const { return FRAME_TYPE_RSP(FRAME_TYPE_STATE_GET); }

void Tion3sApi::read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
  // invalid size is never possible
  // if (frame_data_size != sizeof(tion3s_state_t)) {
  //   TION_LOGW(TAG, "Incorrect state data size: %zu", frame_data_size);
  //   return;
  // }

  if (frame_type == FRAME_TYPE_RSP(FRAME_TYPE_STATE_GET)) {
    TION_LOGD(TAG, "Response[] State Get");
    if (this->on_state) {
      this->on_state(*static_cast<const tion3s_state_t *>(frame_data), 0);
    }
  } else if (frame_type == FRAME_TYPE_RSP(FRAME_TYPE_STATE_SET)) {
    TION_LOGD(TAG, "Response[] State Set");
    if (this->on_state) {
      this->on_state(*static_cast<const tion3s_state_t *>(frame_data), 0);
    }
  } else if (frame_type == FRAME_TYPE_RSP(FRAME_TYPE_TIMERS_GET)) {
    TION_LOGD(TAG, "Response[] Timers: %s", hexencode(frame_data, frame_data_size).c_str());
    // структура Tion3sTimersResponse
  } else if (frame_type == FRAME_TYPE_RSP(FRAME_TYPE_SRV_MODE_SET)) {
    // есть подозрение, что актуальными являеются первые два байта,
    // остальное условый мусор из предыдущей команды
    //
    // один из ответов на команду сопряжения через удеражние кнопки на бризере
    // B3.50.01.00.08.00.08.00.08.00.00.00.00.00.00.00.00.00.00.5A
    // еще:
    // [17:36:21][V][vport:015]: VTX: 3D.01
    // [17:36:21][V][vport:011]: VRX: B3.10.21.19.02.00.19.19.17.68.01.0F.04.00.1E.00.00.3C.00 (19)
    // [17:37:16][V][vport:015]: VTX: 3D.04
    // [17:37:16][V][vport:011]: VRX: B3.40.11.00.08.00.08.00.08.00.00.00.00.00.00.00.00.00.00 (19)
    // [17:37:21][V][vport:015]: VTX: 3D.01
    // [17:37:21][V][vport:011]: VRX: B3.10.21.19.02.00.19.19.17.68.01.0F.05.00.1E.00.00.3C.00 (19)
    // [17:37:39][V][vport:011]: VRX: B3.50.01.00.02.00.19.19.17.68.01.0F.05.00.1E.00.00.3C.00 (19)
    // [17:38:09][V][vport:011]: VRX: B3.50.00.00.02.00.19.19.17.68.01.0F.05.00.1E.00.00.3C.00 (19)
    // [17:38:16][V][vport:015]: VTX: 3D.04
    // [17:38:16][V][vport:011]: VRX: B3.40.11.00.08.00.08.00.08.00.00.00.00.00.00.00.00.00.00 (19)
    // [17:38:21][V][vport:015]: VTX: 3D.01
    // [17:38:21][V][vport:011]: VRX: B3.10.21.19.02.00.19.19.17.68.01.0F.06.00.1E.00.00.3C.00 (19)
    TION_LOGD(TAG, "Response[] Pair: %s", hexencode(frame_data, frame_data_size).c_str());
  } else {
    TION_LOGW(TAG, "Unsupported frame %04X: %s", frame_type, hexencode(frame_data, frame_data_size).c_str());
  }
}

bool Tion3sApi::pair() const {
  TION_LOGD(TAG, "Request[] Pair");
  const struct {
    uint8_t pair;
  } PACKED pair{.pair = 1};
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_SRV_MODE_SET), pair);
}

bool Tion3sApi::request_state() const {
  TION_LOGD(TAG, "Request[] State Get");
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_STATE_GET));
}

bool Tion3sApi::write_state(const tion3s_state_t &state, uint32_t unused_request_id __attribute__((unused))) const {
  TION_LOGD(TAG, "Request[] State Set");
  if (!state.is_initialized()) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto mode = tion3s_state_set_t::create(state);
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_STATE_SET), mode);
}

bool Tion3sApi::reset_filter(const tion3s_state_t &state) const {
  TION_LOGD(TAG, "Request[] Filter Time Reset");
  if (!state.is_initialized()) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }

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

  auto set = tion3s_state_set_t::create(state);
  set.filter_time.reset = true;
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_STATE_SET), set);
}

bool Tion3sApi::request_command4() const {
  TION_LOGD(TAG, "Request[] Timers");
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_TIMERS_GET));
}

bool Tion3sApi::factory_reset(const tion3s_state_t &state) const {
  TION_LOGD(TAG, "Request[] Factory Reset");
  if (!state.is_initialized()) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto set = tion3s_state_set_t::create(state);
  set.factory_reset = true;
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_STATE_SET), set);
}

}  // namespace tion
}  // namespace dentra
