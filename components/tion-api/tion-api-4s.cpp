#include <cstddef>
#include <cmath>

#include "log.h"
#include "utils.h"
#include "tion-api-4s.h"

namespace dentra {
namespace tion {

static const char *const TAG = "tion-api-4s";

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

  FRAME_TYPE_TEST_SET = 0x3830,  // FRAME_TYPE_CURR_TEST_SET
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
  tion4s_state_t::GatePosition gate_position;
  // температрура нагревателя.
  int8_t target_temperature;
  uint8_t fan_speed;
  // filter time in days
  // TODO возможно должен/может быть uint16_t
  uint32_t filter_time;

  static tion4s_state_set_t create(const tion4s_state_t &state) {
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

struct tion4s_time_t {
  int64_t unix_time;
};

struct tion4s_errors_t {
  enum { ERROR_TYPES_COUNT = 32u };
  uint8_t er[ERROR_TYPES_COUNT];
};

struct tion4s_timers_state_t {
  enum { TIMERS_COUNT = 12 };
  struct {
    bool active : 8;
  } timers[TIMERS_COUNT];
};

struct tion4s_timer_settings_t {
  struct {
    bool monday : 1;
    bool tuesday : 1;
    bool wednesday : 1;
    bool thursday : 1;
    bool friday : 1;
    bool saturday : 1;
    bool sunday : 1;
    bool reserved : 1;
    uint8_t hours;
    uint8_t minutes;
  } schedule;
  struct {
    bool power_state : 1;
    bool sound_state : 1;
    bool led_state : 1;
    bool heater_state : 1;
    bool timer_state : 1;
    uint8_t reserved : 3;
  };
  int8_t target_temperature;
  uint8_t fan_state;
  uint8_t device_mode;
};

#pragma pack(pop)

float tion4s_state_t::heater_power() const {
  if (heater_var == 0 || !flags.heater_state) {
    return 0.0f;
  }
  switch (flags.heater_present) {
    case HEATER_PRESENT_NONE:
      return 0.0f;
    case HEATER_PRESENT_1000W:
      return heater_var * (0.01f * 1000.0f);
    case HEATER_PRESENT_1400W:
      return heater_var * (0.01f * 1400.0f);
    default:
      TION_LOGW(TAG, "unknown heater_present value %u", flags.heater_present);
      return NAN;
  }
}

uint16_t TionApi4s::get_state_type() const { return FRAME_TYPE_STATE_RSP; }

bool TionApi4s::read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
  // do not use switch statement with non-contiguous values, as this will generate a lookup table with wasted space.

  if (frame_type == FRAME_TYPE_HEARTBIT_RSP) {
    if (frame_data_size != sizeof(tion_heartbeat_t)) {
      TION_LOGW(TAG, "Incorrect heartbeat response data size: %u", frame_data_size);
    } else {
      this->on_heartbeat(*static_cast<const tion_heartbeat_t *>(frame_data));
    }
    return true;
  }

  if (frame_type == FRAME_TYPE_STATE_RSP) {
    struct state_frame_t {
      uint32_t request_id;
      tion4s_state_t state;
    } PACKED;
    if (frame_data_size != sizeof(state_frame_t)) {
      TION_LOGW(TAG, "Incorrect state response data size: %u", frame_data_size);
    } else {
      auto frame = static_cast<const state_frame_t *>(frame_data);
      this->on_state(frame->state, frame->request_id);
    }
    return true;
  }

  if (frame_type == FRAME_TYPE_TURBO_RSP) {
    struct turbo_frame_t {
      uint32_t request_id;
      tion4s_turbo_t turbo;
    } PACKED;
    if (frame_data_size != sizeof(turbo_frame_t)) {
      TION_LOGW(TAG, "Incorrect turbo response data size: %u", frame_data_size);
    } else {
      auto frame = static_cast<const turbo_frame_t *>(frame_data);
      this->on_turbo(frame->turbo, frame->request_id);
    }
    return true;
  }

  if (frame_type == FRAME_TYPE_DEV_STATUS_RSP) {
    if (frame_data_size != sizeof(tion_dev_status_t)) {
      TION_LOGW(TAG, "Incorrect device status response data size: %u", frame_data_size);
    } else {
      this->on_dev_status(*static_cast<const tion_dev_status_t *>(frame_data));
    }
    return true;
  }

  if (frame_type == FRAME_TYPE_TIME_RSP) {
    struct time_frame_t {
      uint32_t request_id;
      tion4s_time_t time;
    } PACKED;
    if (frame_data_size != sizeof(time_frame_t)) {
      TION_LOGW(TAG, "Incorrect time response data size: %u", frame_data_size);
    } else {
      auto frame = static_cast<const time_frame_t *>(frame_data);
      this->on_time(frame->time.unix_time, frame->request_id);
    }
    return true;
  }

  if (frame_type == FRAME_TYPE_TIMER_RSP) {
    struct timer_settings_frame_t {
      uint32_t request_id;
      uint8_t timer_id;
      tion4s_timer_settings_t timer;
    } PACKED;
    if (frame_data_size != sizeof(timer_settings_frame_t)) {
      TION_LOGW(TAG, "Incorrect timer response data size: %u", frame_data_size);
    } else {
      auto frame = static_cast<const timer_settings_frame_t *>(frame_data);
      // this->on_timer(frame->timer_id, frame->timer, frame->request_id);
    }
    return true;
  }

  if (frame_type == FRAME_TYPE_TIMERS_STATE_RSP) {
    struct timers_state_frame_t {
      uint32_t request_id;
      tion4s_timers_state_t timers_state;
    } PACKED;
    if (frame_data_size != sizeof(timers_state_frame_t)) {
      TION_LOGW(TAG, "Incorrect timers state response data size: %u", frame_data_size);
    } else {
      auto frame = static_cast<const timers_state_frame_t *>(frame_data);
      // this->on_timers_state(frame->timers_state, frame->request_id);
    }
    return true;
  }

  if (frame_type == FRAME_TYPE_ERR_CNT_RSP) {
    struct error_frame_t {
      uint32_t request_id;
      tion4s_errors_t errors;
    } PACKED;
    if (frame_data_size != sizeof(error_frame_t)) {
      TION_LOGW(TAG, "Incorrect error response data size: %u", frame_data_size);
    } else {
      auto frame = *static_cast<const error_frame_t *>(frame_data);
      // this->on_errors(frame->errors, frame->request_id);
    }
    return true;
  }

  if (frame_type == FRAME_TYPE_TEST_RSP) {
    if (frame_data_size != sizeof(uint32_t)) {
      TION_LOGW(TAG, "Incorrect test response data size: %u", frame_data_size);
    } else {
      // auto test_type = static_cast<const uint32_t *>(frame_data);
      // TION_LOGV(TAG, "test_type: %u", test_type);
    }
    return true;
  }

  TION_LOGW(TAG, "Unsupported frame type 0x%04X: %s", frame_type, hexencode(frame_data, frame_data_size).c_str());
  return false;
}

bool TionApi4s::request_dev_status() const {
  TION_LOGD(TAG, "Request[] Device status");
  return this->write_frame(FRAME_TYPE_DEV_STATUS_REQ);
}

bool TionApi4s::request_state() const {
  TION_LOGD(TAG, "Request[] State");
  return this->write_frame(FRAME_TYPE_STATE_REQ);
}

bool TionApi4s::request_turbo() const {
  TION_LOGD(TAG, "Request[] Turbo");
  // TODO проверить, возможно необходимо/можно послыать request_id
  return this->write_frame(FRAME_TYPE_TURBO_REQ);
}

bool TionApi4s::request_time(const uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%u] Time", request_id);
  return this->write_frame(FRAME_TYPE_TIME_REQ, request_id);
}

bool TionApi4s::request_errors() const {
  TION_LOGD(TAG, "Request[] Errors");
  return this->write_frame(FRAME_TYPE_ERR_CNT_REQ);
}

bool TionApi4s::request_test() const {
  TION_LOGD(TAG, "Request[] Test");
  return this->write_frame(FRAME_TYPE_TEST_REQ);
}

bool TionApi4s::request_timers(const uint32_t request_id) const {
  bool res = true;
  for (uint8_t timer_id = 0; timer_id < tion4s_timers_state_t::TIMERS_COUNT; timer_id++) {
    res &= this->request_timer(timer_id, request_id);
  }
  return res;
}

bool TionApi4s::request_timer(const uint8_t timer_id, const uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%u] Timer", request_id);
  struct {
    uint8_t timer_id;
  } PACKED timer{.timer_id = timer_id};
  return this->write_frame(FRAME_TYPE_TIMER_REQ, timer, request_id);
}

bool TionApi4s::write_state(const tion4s_state_t &state, const uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%u] Write state", request_id);
  if (state.counters.work_time == 0) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tion4s_state_set_t::create(state);
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

bool TionApi4s::reset_filter(const tion4s_state_t &state, const uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%u] Reset filter", request_id);
  if (state.counters.work_time == 0) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tion4s_state_set_t::create(state);
  st_set.filter_reset = true;
  st_set.filter_time = 0;
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

bool TionApi4s::factory_reset(const tion4s_state_t &state, const uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%u] Factory reset", request_id);
  if (state.counters.work_time == 0) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tion4s_state_set_t::create(state);
  st_set.factory_reset = true;
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

bool TionApi4s::set_turbo_time(const uint16_t time, const uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%u] Set turbo time %u", request_id, time);
  struct {
    uint16_t time;
    uint8_t err_code;
  } PACKED turbo{.time = time, .err_code = 0};
  return this->write_frame(FRAME_TYPE_TURBO_SET, turbo, request_id);
};

bool TionApi4s::set_time(const time_t time, const uint32_t request_id) const {
#if INTPTR_MAX == INT32_MAX
  TION_LOGD(TAG, "Request[%u] Set time %ld", request_id, time);
#else
  TION_LOGD(TAG, "Request[%u] Set time %lld", request_id, time);
#endif
  tion4s_time_t tm{.unix_time = time};
  return this->write_frame(FRAME_TYPE_TIME_SET, tm, request_id);
}

bool TionApi4s::send_heartbeat() const {
  TION_LOGV(TAG, "Request[] Heartbeat");
  return this->write_frame(FRAME_TYPE_HEARTBIT_REQ);
}

}  // namespace tion
}  // namespace dentra
