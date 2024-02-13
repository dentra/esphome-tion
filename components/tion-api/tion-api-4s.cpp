#include <cstddef>
#include <cmath>
#include <cinttypes>

#include "log.h"
#include "utils.h"
#include "tion-api-4s-internal.h"

namespace dentra {
namespace tion {

using namespace tion_4s;

static const char *const TAG = "tion-api-4s";

void tion4s_state_t::for_each_error(const std::function<void(uint8_t error, const char type[3])> &fn) const {
  if (this->errors == 0) {
    return;
  }
  for (uint32_t i = tion4s_state_t::ERROR_MIN_BIT; i <= tion4s_state_t::ERROR_MAX_BIT; i++) {
    uint32_t mask = 1 << i;
    if ((this->errors & mask) == mask) {
      fn(i + 1, "EC");
    }
  }
  for (uint32_t i = tion4s_state_t::WARNING_MIN_BIT; i <= tion4s_state_t::WARNING_MAX_BIT; i++) {
    uint32_t mask = 1 << i;
    if ((this->errors & mask) == mask) {
      fn(i + 1, "WS");
    }
  }
}

float tion4s_state_t::heater_power() const {
  if (this->heater_var == 0 || !this->flags.heater_state) {
    return 0.0f;
  }
  switch (this->flags.heater_present) {
    case HEATER_PRESENT_NONE:
      return 0.0f;
    case HEATER_PRESENT_1000W:
      return this->heater_var * (0.01f * 1000.0f);
    case HEATER_PRESENT_1400W:
      return this->heater_var * (0.01f * 1400.0f);
    default:
      TION_LOGW(TAG, "unknown heater_present value %u", this->flags.heater_present);
      return NAN;
  }
}

uint16_t TionApi4s::get_state_type() const { return FRAME_TYPE_STATE_RSP; }

void TionApi4s::read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
  // do not use switch statement with non-contiguous values, as this will generate a lookup table with wasted space.
#ifdef TION_ENABLE_HEARTBEAT
  if (frame_type == FRAME_TYPE_HEARTBIT_RSP) {
    struct RawHeartbeatFrame {
      tion_dev_info_t::work_mode_t work_mode;  // always 1
    } PACKED;
    if (frame_data_size != sizeof(RawHeartbeatFrame)) {
      TION_LOGW(TAG, "Incorrect heartbeat response data size: %zu", frame_data_size);
    } else {
      auto *frame = static_cast<const RawHeartbeatFrame *>(frame_data);
      TION_LOGD(TAG, "Response[] Heartbeat (%u)", frame->work_mode);
      if (this->on_heartbeat) {
        this->on_heartbeat(frame->work_mode);
      }
    }
    return;
  }
#endif
  if (frame_type == FRAME_TYPE_STATE_RSP) {
    struct RawStateFrame {
      uint32_t request_id;
      tion4s_state_t state;
    } PACKED;
    if (frame_data_size != sizeof(RawStateFrame)) {
      TION_LOGW(TAG, "Incorrect state response data size: %zu", frame_data_size);
    } else {
      auto *frame = static_cast<const RawStateFrame *>(frame_data);
      TION_LOGD(TAG, "Response[%" PRIu32 "] %s", frame->request_id, frame->request_id == 1 ? "State" : "Write State");
      if (this->on_state) {
        this->on_state(frame->state, frame->request_id);
      }
    }
    return;
  }
#ifdef TION_ENABLE_PRESETS
  if (frame_type == FRAME_TYPE_TURBO_RSP) {
    struct RawTurboFrame {
      uint32_t request_id;
      tion4s_turbo_t turbo;
    } PACKED;
    if (frame_data_size != sizeof(RawTurboFrame)) {
      TION_LOGW(TAG, "Incorrect turbo response data size: %zu", frame_data_size);
    } else {
      auto *frame = static_cast<const RawTurboFrame *>(frame_data);
      TION_LOGD(TAG, "Response[%" PRIu32 "] Turbo", frame->request_id);
      if (this->on_turbo) {
        this->on_turbo(frame->turbo, frame->request_id);
      }
    }
    return;
  }
#endif
  if (frame_type == FRAME_TYPE_DEV_INFO_RSP) {
    if (frame_data_size != sizeof(tion_dev_info_t)) {
      TION_LOGW(TAG, "Incorrect device info response data size: %zu", frame_data_size);
    } else {
      TION_LOGD(TAG, "Response[] Device info");
      if (this->on_dev_info) {
        this->on_dev_info(*static_cast<const tion_dev_info_t *>(frame_data));
      }
    }
    return;
  }
#ifdef TION_ENABLE_SCHEDULER
  if (frame_type == FRAME_TYPE_TIME_RSP) {
    struct RawTimeFrame {
      uint32_t request_id;
      tion4s_time_t time;
    } PACKED;
    if (frame_data_size != sizeof(RawTimeFrame)) {
      TION_LOGW(TAG, "Incorrect time response data size: %zu", frame_data_size);
    } else {
      auto *frame = static_cast<const RawTimeFrame *>(frame_data);
      TION_LOGD(TAG, "Response[%" PRIu32 "] Time", frame->request_id);
      if (this->on_time) {
        this->on_time(frame->time.unix_time, frame->request_id);
      }
    }
    return;
  }

  if (frame_type == FRAME_TYPE_TIMER_RSP) {
    struct RawTimerFrame {
      uint32_t request_id;
      uint8_t timer_id;
      tion4s_timer_t timer;
    } PACKED;
    if (frame_data_size != sizeof(RawTimerFrame)) {
      TION_LOGW(TAG, "Incorrect timer response data size: %zu", frame_data_size);
    } else {
      auto *frame = static_cast<const RawTimerFrame *>(frame_data);
      TION_LOGD(TAG, "Response[%" PRIu32 "] Timer %u", frame->request_id, frame->timer_id);
      if (this->on_timer) {
        this->on_timer(frame->timer_id, frame->timer, frame->request_id);
      }
    }
    return;
  }

  if (frame_type == FRAME_TYPE_TIMERS_STATE_RSP) {
    struct RawTimersStateFrame {
      uint32_t request_id;
      tion4s_timers_state_t timers_state;
    } PACKED;
    if (frame_data_size != sizeof(RawTimersStateFrame)) {
      TION_LOGW(TAG, "Incorrect timers state response data size: %zu", frame_data_size);
    } else {
      auto *frame = static_cast<const RawTimersStateFrame *>(frame_data);
      TION_LOGD(TAG, "Response[%" PRIu32 "] Timers state", frame->request_id);
      if (this->on_timers_state) {
        this->on_timers_state(frame->timers_state, frame->request_id);
      }
    }
    return;
  }
#endif
#ifdef TION_ENABLE_DIAGNOSTIC
  if (frame_type == FRAME_TYPE_ERR_CNT_RSP) {
    struct RawErrorFrame {
      uint32_t request_id;
      tion4s_errors_t errors;
    } PACKED;
    if (frame_data_size != sizeof(RawErrorFrame)) {
      TION_LOGW(TAG, "Incorrect error response data size: %zu", frame_data_size);
    } else {
      auto *frame = static_cast<const RawErrorFrame *>(frame_data);
      TION_LOGD(TAG, "Response[%" PRIu32 "] Errors", frame->request_id);
      // this->on_errors(frame->errors, frame->request_id);
    }
    return;
  }

  if (frame_type == FRAME_TYPE_TEST_RSP) {
    struct RawTestFrame {
      uint8_t unknown[440];
    } PACKED;
    if (frame_data_size != sizeof(RawTestFrame)) {
      TION_LOGW(TAG, "Incorrect test response data size: %zu", frame_data_size);
    } else {
      TION_LOGD(TAG, "Response[] Test");
    }
    return;
  }
#endif
  TION_LOGW(TAG, "Unsupported frame %04X: %s", frame_type, hexencode(frame_data, frame_data_size).c_str());
}

bool TionApi4s::request_dev_info() const {
  TION_LOGD(TAG, "Request[] Device info");
  return this->write_frame(FRAME_TYPE_DEV_INFO_REQ);
}

bool TionApi4s::request_state() const {
  TION_LOGD(TAG, "Request[] State");
  return this->write_frame(FRAME_TYPE_STATE_REQ);
}

#ifdef TION_ENABLE_PRESETS
bool TionApi4s::request_turbo() const {
  TION_LOGD(TAG, "Request[] Turbo");
  // TODO проверить, возможно необходимо/можно послыать request_id
  return this->write_frame(FRAME_TYPE_TURBO_REQ);
}
#endif
bool TionApi4s::write_state(const tion4s_state_t &state, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%" PRIu32 "] Write state", request_id);
  if (!state.is_initialized()) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tion4s_state_set_t::create(state);
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

bool TionApi4s::reset_filter(const tion4s_state_t &state, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%" PRIu32 "] Reset filter", request_id);
  if (!state.is_initialized()) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tion4s_state_set_t::create(state);
  st_set.filter_reset = true;
  st_set.filter_time = 0;
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

bool TionApi4s::factory_reset(const tion4s_state_t &state, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%" PRIu32 "] Factory reset", request_id);
  if (!state.is_initialized()) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tion4s_state_set_t::create(state);
  st_set.factory_reset = true;
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

#ifdef TION_ENABLE_PRESETS
bool TionApi4s::set_turbo(uint16_t time, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%" PRIu32 "] Turbo %u", request_id, time);
  const struct {
    uint16_t time;
    uint8_t err_code;
  } PACKED turbo{.time = time, .err_code = 0};
  return this->write_frame(FRAME_TYPE_TURBO_SET, turbo, request_id);
};
#endif

#ifdef TION_ENABLE_HEARTBEAT
bool TionApi4s::send_heartbeat() const {
  TION_LOGD(TAG, "Request[] Heartbeat");
  return this->write_frame(FRAME_TYPE_HEARTBIT_REQ);
}
#endif

#ifdef TION_ENABLE_SCHEDULER
bool TionApi4s::request_time(uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%" PRIu32 "] Time", request_id);
  return this->write_frame(FRAME_TYPE_TIME_REQ, request_id);
}

bool TionApi4s::request_timer(uint8_t timer_id, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%" PRIu32 "] Timer %u", request_id, timer_id);
  const struct {
    uint8_t timer_id;
  } PACKED timer{.timer_id = timer_id};
  return this->write_frame(FRAME_TYPE_TIMER_REQ, timer, request_id);
}

bool TionApi4s::request_timers(uint32_t request_id) const {
  bool res = true;
  for (uint8_t timer_id = 0; timer_id < tion4s_timers_state_t::TIMERS_COUNT; timer_id++) {
    res &= this->request_timer(timer_id, request_id);
  }
  return res;
}

bool TionApi4s::write_timer(uint8_t timer_id, const tion4s_timer_t &timer, uint32_t request_id) const {
  const struct RawTimerSet {
    uint8_t timer_id;
    tion4s_timer_t timer;
  } PACKED set{.timer_id = timer_id, .timer = timer};
  return this->write_frame(FRAME_TYPE_TIMER_SET, set, request_id);
}

bool TionApi4s::request_timers_state(const uint32_t request_id) const {
  return this->write_frame(FRAME_TYPE_TIMERS_STATE_REQ, request_id);
}

bool TionApi4s::set_time(time_t time, uint32_t request_id) const {
  const tion4s_time_t tm{.unix_time = time};
  TION_LOGD(TAG, "Request[%" PRIu32 "] Time %lld", request_id, tm.unix_time);
  return this->write_frame(FRAME_TYPE_TIME_SET, tm, request_id);
}
#endif
#ifdef TION_ENABLE_DIAGNOSTIC
bool TionApi4s::request_errors() const {
  TION_LOGD(TAG, "Request[] Errors");
  return this->write_frame(FRAME_TYPE_ERR_CNT_REQ);
}

bool TionApi4s::request_test() const {
  TION_LOGD(TAG, "Request[] Test");
  return this->write_frame(FRAME_TYPE_TEST_REQ);
}
#endif

bool TionApi4s::reset_errors(const tion4s_state_t &state, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%" PRIu32 "] Errors reset", request_id);
  if (!state.is_initialized()) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tion4s_state_set_t::create(state);
  st_set.error_reset = true;
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

}  // namespace tion
}  // namespace dentra
