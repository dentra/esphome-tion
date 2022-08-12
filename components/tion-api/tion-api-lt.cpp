#include <cmath>

#include "log.h"
#include "utils.h"
#include "tion-api-lt.h"

namespace dentra {
namespace tion {

static const char *const TAG = "tion-api-lt";

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
  tionlt_state_t::button_presets_t button_presets;
  uint16_t filter_time;
  uint8_t test_type;

  static tionlt_state_set_t create(const tionlt_state_t &state) {
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

float tionlt_state_t::heater_power() const { return flags.heater_present ? heater_var * (0.01f * 1000.0f) : 0.0f; }

uint16_t TionApiLt::get_state_type() const { return FRAME_TYPE_STATE_RSP; }

bool TionApiLt::read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
  // do not use switch statement with non-contiguous values, as this will generate a lookup table with wasted space.
  if (frame_type == FRAME_TYPE_STATE_RSP) {
    struct state_frame_t {
      uint32_t request_id;
      tionlt_state_t state;
    } PACKED;
    if (frame_data_size != sizeof(state_frame_t)) {
      TION_LOGW(TAG, "Incorrect state response data size: %u", frame_data_size);
    } else {
      auto frame = static_cast<const state_frame_t *>(frame_data);
      this->on_state(frame->state, frame->request_id);
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

  if (frame_type == FRAME_TYPE_AUTOKIV_PARAM_RSP) {
    TION_LOGW(TAG, "FRAME_TYPE_AUTOKIV_PARAM_RSP response: %s", hexencode(frame_data, frame_data_size).c_str());
    return true;
  }

  TION_LOGW(TAG, "Unsupported frame type 0x%04X: %s", frame_type, hexencode(frame_data, frame_data_size).c_str());
  return false;
}

bool TionApiLt::request_dev_status() const {
  TION_LOGD(TAG, "Request[] device status");
  return this->write_frame(FRAME_TYPE_DEV_STATUS_REQ);
}

bool TionApiLt::request_state() const {
  TION_LOGD(TAG, "Request[] state");
  return this->write_frame(FRAME_TYPE_STATE_REQ);
}

bool TionApiLt::write_state(const tionlt_state_t &state, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%u] Write state", request_id);
  if (state.counters.work_time == 0) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tionlt_state_set_t::create(state);
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

bool TionApiLt::reset_filter(const tionlt_state_t &state, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%u] Reset filter", request_id);
  if (state.counters.work_time == 0) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tionlt_state_set_t::create(state);
  st_set.filter_reset = true;
  st_set.filter_time = 0;
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

bool TionApiLt::factory_reset(const tionlt_state_t &state, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%u] Factory reset", request_id);
  if (state.counters.work_time == 0) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tionlt_state_set_t::create(state);
  st_set.factory_reset = true;
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

}  // namespace tion
}  // namespace dentra
