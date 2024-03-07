#include <cmath>
#include <cinttypes>

#include "log.h"
#include "utils.h"
#include "tion-api-lt.h"
#include "tion-api-defines.h"

namespace dentra {
namespace tion {

using tion::TionTraits;
using tion::TionState;
using tion_lt::tionlt_state_t;
using tion_lt::tionlt_state_set_t;
using namespace tion_lt;

static const char *const TAG = "tion-api-lt";

uint16_t TionLtApi::get_state_type() const { return FRAME_TYPE_STATE_RSP; }

void TionLtApi::read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
  // do not use switch statement with non-contiguous values, as this will generate a lookup table with wasted space.
  if (frame_type == FRAME_TYPE_STATE_RSP) {
    // NOLINTNEXTLINE(readability-identifier-naming)
    struct state_frame_t {
      uint32_t request_id;
      tionlt_state_t state;
    } PACKED;
    if (frame_data_size != sizeof(state_frame_t)) {
      TION_LOGW(TAG, "Incorrect state response data size: %zu", frame_data_size);
    } else {
      const auto *frame = static_cast<const state_frame_t *>(frame_data);
      TION_LOGD(TAG, "Response[%" PRIu32 "] State", frame->request_id);
      this->update_state_(frame->state);
      this->notify_state_(frame->request_id);
    }
    return;
  }

  if (frame_type == FRAME_TYPE_DEV_INFO_RSP) {
    if (frame_data_size != sizeof(tion_dev_info_t)) {
      TION_LOGW(TAG, "Incorrect device info response data: %s", hexencode(frame_data, frame_data_size).c_str());
    } else {
      TION_LOGD(TAG, "Response[] Device info");
      const auto *frame = static_cast<const tion_dev_info_t *>(frame_data);
      this->update_dev_info_(*frame);
      this->on_dev_info_fn.call_if(*frame);
    }
    return;
  }

  if (frame_type == FRAME_TYPE_AUTOKIV_PARAM_RSP) {
    TION_LOGD(TAG, "auto kiv param response: %s", hexencode(frame_data, frame_data_size).c_str());
    return;
  }

  TION_LOGW(TAG, "Unsupported frame type 0x%04X: %s", frame_type, hexencode(frame_data, frame_data_size).c_str());
}

bool TionLtApi::request_dev_info_() const {
  TION_LOGD(TAG, "Request[] device info");
  return this->write_frame(FRAME_TYPE_DEV_INFO_REQ);
}

bool TionLtApi::request_state_() const {
  TION_LOGD(TAG, "Request[] state");
  return this->write_frame(FRAME_TYPE_STATE_REQ);
}

bool TionLtApi::write_state(const TionState &state, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%" PRIu32 "] Write state", request_id);
  if (!state.is_initialized(this->traits_)) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tionlt_state_set_t::create(state, this->button_presets_);
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

bool TionLtApi::reset_filter(const TionState &state, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%" PRIu32 "] Reset filter", request_id);
  if (!state.is_initialized(this->traits_)) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tionlt_state_set_t::create(state, this->button_presets_);
  st_set.filter_reset = true;
  st_set.filter_time = 181;
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

bool TionLtApi::factory_reset(const TionState &state, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%" PRIu32 "] Factory reset", request_id);
  if (!state.is_initialized(this->traits_)) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tionlt_state_set_t::create(state, this->button_presets_);
  st_set.factory_reset = true;
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

bool TionLtApi::reset_errors(const TionState &state, uint32_t request_id) const {
  TION_LOGD(TAG, "Request[%" PRIu32 "] Error reset", request_id);
  if (!state.is_initialized(this->traits_)) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tionlt_state_set_t::create(state, this->button_presets_);
  st_set.error_reset = true;
  return this->write_frame(FRAME_TYPE_STATE_SET, st_set, request_id);
}

void TionLtApi::request_state() {
  if (this->state_.firmware_version == 0) {
    this->request_dev_info_();
  }
  this->request_state_();
}

TionLtApi::TionLtApi() {
  this->traits_.errors_decoder = tionlt_state_t::decode_errors;

  this->traits_.supports_heater_var = true;
  this->traits_.supports_work_time = true;
  this->traits_.supports_airflow_counter = true;
  this->traits_.supports_fan_time = true;
  this->traits_.supports_led_state = true;
  this->traits_.supports_sound_state = true;
  this->traits_.supports_pcb_pwr_temperature = true;
  this->traits_.max_heater_power = TION_LT_HEATER_POWER / 10;
  this->traits_.max_fan_speed = 6;
  this->traits_.min_target_temperature = TION_MIN_TEMPERATURE;
  this->traits_.max_target_temperature = TION_MAX_TEMPERATURE;

  this->traits_.max_fan_power[0] = TION_LT_MAX_FAN_POWER0;
  this->traits_.max_fan_power[1] = TION_LT_MAX_FAN_POWER1;
  this->traits_.max_fan_power[2] = TION_LT_MAX_FAN_POWER2;
  this->traits_.max_fan_power[3] = TION_LT_MAX_FAN_POWER3;
  this->traits_.max_fan_power[4] = TION_LT_MAX_FAN_POWER4;
  this->traits_.max_fan_power[5] = TION_LT_MAX_FAN_POWER5;
  this->traits_.max_fan_power[6] = TION_LT_MAX_FAN_POWER6;
}

void TionLtApi::update_dev_info_(const tion::tion_dev_info_t &dev_info) {
  this->state_.firmware_version = dev_info.firmware_version;
  this->state_.hardware_version = dev_info.hardware_version;
}

void TionLtApi::update_state_(const tionlt_state_t &state) {
  this->traits_.initialized = true;

  this->state_.power_state = state.power_state;
  this->state_.heater_state = state.heater_state;
  this->state_.sound_state = state.sound_state;
  this->state_.led_state = state.led_state;
  this->state_.comm_source = state.comm_source;
  this->state_.auto_state = state.ma_auto;
  this->state_.filter_state = state.filter_state;
  // this->state_.gate_error_state = 0;
  this->state_.gate_position = state.gate_state ? TionGatePosition::OPENED : TionGatePosition::CLOSED;
  this->state_.fan_speed = state.fan_speed;
  this->state_.outdoor_temperature = state.outdoor_temperature;
  this->state_.current_temperature = state.current_temperature;
  this->state_.target_temperature = state.target_temperature;
  this->state_.productivity = this->calc_productivity(state);
  this->state_.heater_var = state.heater_var;
  this->state_.work_time = state.counters.work_time;
  this->state_.fan_time = state.counters.fan_time;
  this->state_.filter_time_left = state.counters.filter_time;
  this->state_.airflow_counter = state.counters.airflow_counter;
  this->state_.airflow_m3 = state.counters.airflow();
  this->traits_.max_heater_power = state.heater_present ? TION_LT_HEATER_POWER / 10 : 0;
  this->traits_.max_fan_speed = state.max_fan_speed;
  // this->traits_.min_target_temperature = -30;
  // this->traits_.min_target_temperature = 25;
  // this->state_.hardware_version = dev_info.hardware_version;
  // this->state_.firmware_version = dev_info.firmware_version;
  this->state_.pcb_ctl_temperature = state.pcb_temperature;
  this->state_.pcb_pwr_temperature = state.pcb_temperature;
  this->state_.errors = state.errors;

  this->dump_state_(state);
}

void TionLtApi::dump_state_(const tionlt_state_t &state) const {
  this->state_.dump(TAG, this->traits_);

  TION_LOGV(TAG, "airflow_counter: %" PRIu32, state.counters.airflow_counter);

  TION_LOGV(TAG, "heater_present : %s", ONOFF(state.heater_present));
  TION_LOGV(TAG, "heater_var     : %u", state.heater_var);
  TION_LOGV(TAG, "kiv_present    : %s", ONOFF(state.kiv_present));
  TION_LOGV(TAG, "kiv_active     : %s", ONOFF(state.kiv_active));
  TION_LOGV(TAG, "reserved       : %02x", state.reserved);
  TION_LOGV(TAG, "gate_state     : %u", state.gate_state);
  TION_LOGV(TAG, "pcb_temperature: %u", state.pcb_temperature);
  TION_LOGV(TAG, "fan_time       : %" PRIu32, state.counters.fan_time);
  TION_LOGV(TAG, "errors_cnt     : %s",
            format_hex_pretty(reinterpret_cast<const uint8_t *>(&state.errors_cnt), sizeof(state.errors_cnt)).c_str());
  TION_LOGV(TAG, "button_presets : %d/%d/%d, %d/%d/%d °C",             //-//
            state.button_presets.fan[0], state.button_presets.fan[1],  //-//
            state.button_presets.fan[2], state.button_presets.tmp[0],  //-//
            state.button_presets.tmp[1], state.button_presets.tmp[2]);
  TION_LOGV(TAG, "test_type      : %u", state.test_type);
}

void TionLtApi::set_button_presets(const dentra::tion_lt::button_presets_t &button_presets) {
  this->button_presets_ = button_presets;
}

}  // namespace tion
}  // namespace dentra
