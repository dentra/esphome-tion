#include <inttypes.h>
#include <cmath>

#include "log.h"
#include "utils.h"
#include "tion-api-4s.h"

namespace dentra {
namespace tion {

static const char *const TAG = "tion-api-4s";

enum {
  NUM_OF_TIMERS = 6,
};

enum {
  FRAME_TYPE_STATE_SET = 0x3230,  // no save req
  FRAME_TYPE_STATE_RSP = 0x3231,
  FRAME_TYPE_STATE_REQ = 0x3232,
  FRAME_TYPE_STATE_SAV = 0x3234,  // save req

  FRAME_TYPE_DEV_STATUS_REQ = 0x3332,
  FRAME_TYPE_DEV_STATUS_RSP = 0x3331,

  FRAME_TYPE_TEST_REQ = 0x3132,
  FRAME_TYPE_TEST_RSP = 0x3131,

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

  FRAME_TYPE_TURBO_SET = 0x4130,
  FRAME_TYPE_TURBO_REQ = 0x4132,
  FRAME_TYPE_TURBO_RSP = 0x4131,
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

struct tion4s_errors_t {
  enum { ERROR_TYPE_NUMBER = 32u };
  uint8_t er[ERROR_TYPE_NUMBER];
};

struct tion4s_timer_settings_t {
  struct {
    uint8_t monday : 1;
    uint8_t tuesday : 1;
    uint8_t wednesday : 1;
    uint8_t thursday : 1;
    uint8_t friday : 1;
    uint8_t saturday : 1;
    uint8_t sunday : 1;
    uint8_t reserved : 1;
    uint8_t hours;
    uint8_t minutes;
  } timer_time;
  struct {
    uint8_t power_state : 1;
    uint8_t sound_state : 1;
    uint8_t led_state : 1;
    uint8_t heater_mode : 1;  // 0 - temperature maintenance, 1 - heating //!!!отличается от HeaterMode!!!
    uint8_t timer_state : 1;  /* on/off */
    uint8_t reserved : 3;
  } timer_flags;
  int8_t target_temp;
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

void TionApi4s::read_(uint16_t frame_type, const void *frame_data, uint16_t frame_data_size) {
  TION_LOGV(TAG, "read frame data 0x%04X: %s", frame_type, hexencode(frame_data, frame_data_size).c_str());
  // do not use switch statement with non-contiguous values, as this will generate a lookup table with wasted space.
  if (false) {
    // just pretty formatting
  } else if (frame_type == FRAME_TYPE_STATE_RSP) {
    struct state_frame_t {
      uint32_t request_id;
      tion4s_state_t state;
    } PACKED;
    if (frame_data_size != sizeof(state_frame_t)) {
      TION_LOGW(TAG, "Incorrect state response data size: %u", frame_data_size);
    } else {
      this->read(static_cast<const state_frame_t *>(frame_data)->state);
    }
  } else if (frame_type == FRAME_TYPE_DEV_STATUS_RSP) {
    if (frame_data_size != sizeof(tion_dev_status_t)) {
      TION_LOGW(TAG, "Incorrect device status response data size: %u", frame_data_size);
    } else {
      this->read(*static_cast<const tion_dev_status_t *>(frame_data));
    }
  } else if (frame_type == FRAME_TYPE_TURBO_RSP) {
    struct turbo_frame_t {
      uint32_t request_id;
      tion4s_turbo_t turbo;
    } PACKED;
    if (frame_data_size != sizeof(turbo_frame_t)) {
      TION_LOGW(TAG, "Incorrect turbo response data size: %u", frame_data_size);
    } else {
      this->read(static_cast<const turbo_frame_t *>(frame_data)->turbo);
    }
  } else if (frame_type == FRAME_TYPE_TIME_RSP) {
    struct time_frame_t {
      uint32_t request_id;
      tion4s_time_t time;
    } PACKED;
    if (frame_data_size != sizeof(time_frame_t)) {
      TION_LOGW(TAG, "Incorrect time response data size: %u", frame_data_size);
    } else {
      this->read(static_cast<const time_frame_t *>(frame_data)->time);
    }
  } else if (frame_type == FRAME_TYPE_ERR_CNT_RSP) {
    struct error_frame_t {
      uint32_t request_id;
      tion4s_errors_t cnt;
    } PACKED;
    if (frame_data_size != sizeof(error_frame_t)) {
      TION_LOGW(TAG, "Incorrect error response data size: %u", frame_data_size);
    } else {
      // auto &err_data = *static_cast<const error_frame_t *>(frame_data);
      // for (uint32_t i = 0; i < ERROR_TYPE_NUMBER; i++) {
      //   TION_LOGV(TAG, "err_cnt[%u]: %u", i, err_data.cnt.er[i]);
      // }
    }
  } else if (frame_type == FRAME_TYPE_TEST_RSP) {
    if (frame_data_size != sizeof(uint32_t)) {
      TION_LOGW(TAG, "Incorrect test response data size: %u", frame_data_size);
    } else {
      // auto &test_type = *static_cast<const uint32_t *>(frame_data);
      // TION_LOGV(TAG, "test_type: %u", test_type);
    }
  } else if (frame_type == FRAME_TYPE_TIMER_RSP) {
    struct timer_settings_frame_t {
      uint32_t request_id;
      uint8_t timer_id;
      tion4s_timer_settings_t timer;
    } PACKED;
    if (frame_data_size != sizeof(timer_settings_frame_t)) {
      TION_LOGW(TAG, "Incorrect timer response data size: %u", frame_data_size);
    } else {
      // auto &timer_set = *static_cast<const timer_settings_frame_t *>(frame_data);
      // TION_LOGV(TAG, "timer[%u].device_mode: %u", timer_set.timer_id, timer_set.timer.device_mode);
      // TION_LOGV(TAG, "timer[%u].target_temp: %d", timer_set.timer_id, timer_set.timer.target_temp);
      // TION_LOGV(TAG, "timer[%u].fan_state: %u", timer_set.timer_id, timer_set.timer.fan_state);
      // timer[timer_set.timer_id] = timer_set.timer;
    }
  } else if (frame_type == FRAME_TYPE_TIMERS_STATE_RSP) {
    TION_LOGW(TAG, "FRAME_TYPE_TIMERS_STATE_RSP response: %s", hexencode(frame_data, frame_data_size).c_str());
  } else {
    TION_LOGW(TAG, "Unsupported frame type 0x%04X: %s", frame_type, hexencode(frame_data, frame_data_size).c_str());
  }
}

void TionApi4s::request_dev_status() {
  TION_LOGD(TAG, "Request device status");
  this->write_(FRAME_TYPE_DEV_STATUS_REQ);
}

void TionApi4s::request_state() {
  TION_LOGD(TAG, "Request state");
  this->write_(FRAME_TYPE_STATE_REQ);
}

void TionApi4s::request_turbo() {
  TION_LOGD(TAG, "Request turbo");
  this->write_(FRAME_TYPE_TURBO_REQ);
}

void TionApi4s::request_time() {
  TION_LOGD(TAG, "Request time");
  this->write_(FRAME_TYPE_TIME_REQ);
}

void TionApi4s::request_errors() {
  TION_LOGD(TAG, "Request errors");
  this->write_(FRAME_TYPE_ERR_CNT_REQ);
}

void TionApi4s::request_test() {
  TION_LOGD(TAG, "Request test");
  this->write_(FRAME_TYPE_TEST_REQ);
}

void TionApi4s::request_timers() {
  TION_LOGD(TAG, "Request timers");
  for (uint8_t timer_id = 0; timer_id < NUM_OF_TIMERS; timer_id++) {
    this->write_(FRAME_TYPE_TIMER_REQ, timer_id, this->next_command_id());
  }
}

bool TionApi4s::write_state(const tion4s_state_t &state) const {
  TION_LOGD(TAG, "Write state");
  if (state.counters.work_time == 0) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tion4s_state_set_t::create(state);
  return this->write_(FRAME_TYPE_STATE_SET, st_set, this->next_command_id());
}

bool TionApi4s::reset_filter(const tion4s_state_t &state) const {
  TION_LOGD(TAG, "Reset filter");
  if (state.counters.work_time == 0) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tion4s_state_set_t::create(state);
  st_set.filter_reset = true;
  st_set.filter_time = 0;
  return this->write_(FRAME_TYPE_STATE_SET, st_set, this->next_command_id());
}

bool TionApi4s::factory_reset(const tion4s_state_t &state) const {
  TION_LOGD(TAG, "Factory reset");
  if (state.counters.work_time == 0) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tion4s_state_set_t::create(state);
  st_set.factory_reset = true;
  return this->write_(FRAME_TYPE_STATE_SET, st_set, this->next_command_id());
}

bool TionApi4s::set_turbo_time(uint16_t time) const {
  TION_LOGD(TAG, "Set turbo time %u", time);
  struct {
    uint16_t time;
    uint8_t err_code;
  } PACKED turbo{.time = time, .err_code = 0};
  return this->write_(FRAME_TYPE_TURBO_SET, turbo, this->next_command_id());
};

bool TionApi4s::set_time(int64_t time) const {
  TION_LOGD(TAG, "Set time %" PRId64, time);
  tion4s_time_t tm{.unix_time = time};
  return this->write_(FRAME_TYPE_TIME_SET, tm, this->next_command_id());
}

}  // namespace tion
}  // namespace dentra
