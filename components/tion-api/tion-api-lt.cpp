#include <cmath>
#include <cinttypes>

#include "log.h"
#include "utils.h"
#include "tion-api-lt-internal.h"

namespace dentra {
namespace tion {

using namespace tion_lt;

static const char *const TAG = "tion-api-lt";

float tionlt_state_t::heater_power() const { return flags.heater_present ? heater_var * (0.01f * 1000.0f) : 0.0f; }

uint16_t TionApiLt::get_state_type() const { return FRAME_TYPE_STATE_RSP; }

void TionApiLt::read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
  // do not use switch statement with non-contiguous values, as this will generate a lookup table with wasted space.
  if (frame_type == FRAME_TYPE_STATE_RSP) {
    struct state_frame_t {
      uint32_t request_id;
      tionlt_state_t state;
    } PACKED;
    if (frame_data_size != sizeof(state_frame_t)) {
      TION_LOGW(TAG, "Incorrect state response data size: %zu", frame_data_size);
    } else {
      auto frame = static_cast<const state_frame_t *>(frame_data);
      TION_LOGD(TAG, "Response[%u] State", frame->request_id);
      if (this->on_state) {
        this->on_state(frame->state, frame->request_id);
      }
    }
    return;
  }

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

  if (frame_type == FRAME_TYPE_AUTOKIV_PARAM_RSP) {
    TION_LOGW(TAG, "FRAME_TYPE_AUTOKIV_PARAM_RSP response: %s", hexencode(frame_data, frame_data_size).c_str());
    return;
  }

  TION_LOGW(TAG, "Unsupported frame type 0x%04X: %s", frame_type, hexencode(frame_data, frame_data_size).c_str());
}

bool TionApiLt::request_dev_info() const {
  TION_LOGD(TAG, "Request[] device info");
  return this->write_frame(FRAME_TYPE_DEV_INFO_REQ);
}

bool TionApiLt::request_state() const {
  TION_LOGD(TAG, "Request[] state");
  return this->write_frame(FRAME_TYPE_STATE_REQ);
}

bool TionApiLt::write_state(const tionlt_state_t &state, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%" PRIu32 "] Write state", request_id);
  if (!state.is_initialized()) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tionlt_state_set_t::create(state);
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

bool TionApiLt::reset_filter(const tionlt_state_t &state, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%" PRIu32 "] Reset filter", request_id);
  if (!state.is_initialized()) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tionlt_state_set_t::create(state);
  st_set.filter_reset = true;
  st_set.filter_time = 0;
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

bool TionApiLt::factory_reset(const tionlt_state_t &state, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%" PRIu32 "] Factory reset", request_id);
  if (!state.is_initialized()) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tionlt_state_set_t::create(state);
  st_set.factory_reset = true;
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

}  // namespace tion
}  // namespace dentra
