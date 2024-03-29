#include <cstring>
#include <cinttypes>
#include <cstdlib>

#include "log.h"
#include "utils.h"
#include "tion-api-3s.h"
#include "tion-api-defines.h"

namespace dentra {
namespace tion_3s {

using tion::TionTraits;

std::string tion3s_state_t::decode_errors(uint32_t errors) {
  if (errors == 0) {
    return {};
  }
  // Отвалился походу датчик температуры :( кнопка красным мигает
  // EC10 пишет ошибку
  return "EC" + std::to_string(errors);
}

}  // namespace tion_3s
namespace tion {

using namespace tion_3s;

static const char *const TAG = "tion-api-3s";

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
    auto *frame = static_cast<const tion3s_state_t *>(frame_data);
    this->update_state_(*frame);
    this->notify_state_(0);
  } else if (frame_type == FRAME_TYPE_RSP(FRAME_TYPE_STATE_SET)) {
    TION_LOGD(TAG, "Response[] State Set");
    auto *frame = static_cast<const tion3s_state_t *>(frame_data);
    this->update_state_(*frame);
    this->notify_state_(0);
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

bool Tion3sApi::request_state_() const {
  TION_LOGD(TAG, "Request[] State Get");
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_STATE_GET));
}

bool Tion3sApi::write_state(const tion::TionState &state, uint32_t request_id __attribute__((unused))) const {
  TION_LOGD(TAG, "Request[] State Set");
  if (!state.is_initialized(this->traits_)) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tion3s_state_set_t::create(state);
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_STATE_SET), st_set);
}

bool Tion3sApi::reset_filter(const tion::TionState &state) const {
  TION_LOGD(TAG, "Request[] Filter Time Reset");
  if (!state.is_initialized(this->traits_)) {
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

  auto st_set = tion3s_state_set_t::create(state);
  st_set.filter_time.reset = true;
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_STATE_SET), st_set);
}

bool Tion3sApi::request_command4() const {
  TION_LOGD(TAG, "Request[] Timers");
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_TIMERS_GET));
}

bool Tion3sApi::factory_reset(const tion::TionState &state) const {
  TION_LOGD(TAG, "Request[] Factory Reset");
  if (!state.is_initialized(this->traits_)) {
    TION_LOGW(TAG, "State is not initialized");
    return false;
  }
  auto st_set = tion3s_state_set_t::create(state);
  st_set.factory_reset = true;
  return this->write_frame(FRAME_TYPE_REQ(FRAME_TYPE_STATE_SET), st_set);
}

void Tion3sApi::request_state() { this->request_state_(); }

Tion3sApi::Tion3sApi() {
  this->traits_.errors_decoder = tion3s_state_t::decode_errors;

  this->traits_.supports_sound_state = true;
  this->traits_.supports_gate_position_change = true;
  this->traits_.supports_gate_position_change_mixed = true;
#ifdef TION_ENABLE_ANTIFRIZE
  this->traits_.supports_antifrize = true;
#endif
  this->traits_.supports_reset_filter = true;
  this->traits_.max_heater_power = TION_3S_HEATER_POWER / 10;
  this->traits_.max_fan_speed = 6;
  this->traits_.min_target_temperature = TION_MIN_TEMPERATURE;
  this->traits_.max_target_temperature = TION_MAX_TEMPERATURE;

  this->traits_.max_fan_power[0] = TION_3S_MAX_FAN_POWER0;
  this->traits_.max_fan_power[1] = TION_3S_MAX_FAN_POWER1;
  this->traits_.max_fan_power[2] = TION_3S_MAX_FAN_POWER2;
  this->traits_.max_fan_power[3] = TION_3S_MAX_FAN_POWER3;
  this->traits_.max_fan_power[4] = TION_3S_MAX_FAN_POWER4;
  this->traits_.max_fan_power[5] = TION_3S_MAX_FAN_POWER5;
  this->traits_.max_fan_power[6] = TION_3S_MAX_FAN_POWER6;
}

void Tion3sApi::update_state_(const tion_3s::tion3s_state_t &state) {
  this->traits_.initialized = true;

  this->state_.power_state = state.flags.power_state;
  this->state_.heater_state = state.flags.heater_state;
  this->state_.sound_state = state.flags.sound_state;
  // this->state_.led_state = state.led_state;
  // this->state_.comm_source = state.comm_source;
  this->state_.auto_state = state.flags.ma_auto;
  this->state_.filter_state = state.filter_time <= 30;
  // this->state_.gate_error_state = state.errors & tion4s_state_t::GATE_ERROR_BIT;
  this->state_.gate_position =                                                  //-//
      state.gate_position == tion3s_state_t::GATE_POSITION_INDOOR               //-//
          ? TionGatePosition::INDOOR                                            //-//
          : state.gate_position == tion3s_state_t::GATE_POSITION_MIXED          //-//
                ? TionGatePosition::MIXED                                       //-//
                : state.gate_position == tion3s_state_t::GATE_POSITION_OUTDOOR  //-//
                      ? TionGatePosition::OUTDOOR                               //-//
                      : TionGatePosition::UNKNOWN;                              //-//

  this->state_.fan_speed = state.fan_speed;
  this->state_.outdoor_temperature = state.outdoor_temperature;
  this->state_.current_temperature = state.current_temperature();
  this->state_.target_temperature = state.target_temperature;
  this->state_.productivity = state.productivity;
  // this->state_.heater_var = state.heater_var;
  this->state_.work_time = tion_millis() / 1000;
  // this->state_.fan_time = state.counters.fan_time;
  this->state_.filter_time_left = uint32_t(state.filter_time > 360 ? 1 : state.filter_time) * (24 * 3600);
  // this->state_.airflow_counter = state.counters.airflow_counter;
  // this->traits_.heater_present = TION_3S_HEATER_POWER;
  // this->traits_.max_fan_speed = state.max_fan_speed;
  // this->traits_.min_target_temperature = -30;
  // this->traits_.min_target_temperature = 25;
  // this->state_.hardware_version=dev_info.hardware_version;
  this->state_.firmware_version = state.firmware_version;
  // this->state_.pcb_ctl_temperature = state.pcb_ctl_temperature;
  // this->state_.pcb_pwr_temperature = state.pcb_pwr_temperature;
  this->state_.errors = state.last_error;

  this->dump_state_(state);
}

void Tion3sApi::dump_state_(const tion_3s::tion3s_state_t &state) const {
  this->state_.dump(TAG, this->traits_);
  TION_LOGV(TAG, "filter_days : %u", state.filter_days);
  TION_LOGV(TAG, "productivity: %u", state.productivity);
  TION_LOGV(TAG, "current_T1  : %d", state.current_temperature1);
  TION_LOGV(TAG, "current_T2  : %d", state.current_temperature2);
  TION_LOGV(TAG, "timer_state : %s", ONOFF(state.flags.timer_state));
  TION_LOGV(TAG, "preset_state: %s", ONOFF(state.flags.preset_state));
  TION_LOGV(TAG, "save        : %s", ONOFF(state.flags.save));
  TION_LOGV(TAG, "ma_connected: %s", ONOFF(state.flags.ma_connected));
  TION_LOGV(TAG, "ma_pairing  : %s", ONOFF(state.flags.ma_pairing));
  TION_LOGV(TAG, "reserved    : 0x%02X", state.flags.reserved);
  TION_LOGV(TAG, "hours       : %u", state.hours);
  TION_LOGV(TAG, "minutes     : %u", state.minutes);
}

}  // namespace tion
}  // namespace dentra
