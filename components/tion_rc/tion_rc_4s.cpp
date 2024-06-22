#include <cinttypes>
#include "esphome/core/defines.h"
#include "esphome/core/log.h"

#include "../tion-api/tion-api-4s-internal.h"

#include "tion_rc_4s.h"

namespace esphome {
namespace tion_rc {

static const char *const TAG = "tion_rc_4s";

#ifndef TION_RC_4S_DUMP
#define TION_RC_4S_DUMP ESP_LOGV
#endif

using namespace dentra::tion;
using namespace dentra::tion_4s;

#define BLE_SERVICE_NAME "Breezer 4S"

template<typename T, size_t V> struct BleAdvField {
  uint8_t size{sizeof(BleAdvField<T, V>) - 1};
  uint8_t type{V};
  T data;
  BleAdvField(const T data) {
    if constexpr (std::is_array<T>::value) {
      memcpy(this->data, data, sizeof(T));
    } else {
      this->data = data;
    }
  }
#ifndef __clang__
  BleAdvField(const char *data) { memcpy(this->data, data, sizeof(T)); }
#endif
} PACKED;

void Tion4sRC::adv(bool pair) {
  esp_ble_gap_set_device_name(BLE_SERVICE_NAME);

  struct TionAdvManuData {
    uint16_t vnd_id;
    uint8_t mac[ESP_BD_ADDR_LEN];
    tion_dev_info_t::device_type_t type;
    struct {
      bool pair : 8;
    };
  } PACKED;
  static_assert(sizeof(TionAdvManuData) == 13);

  // для пульта важна последовательность следования данныйх внутри "рекламы", требуется:
  // flags, manu, name. esp_ble_gap_config_adv_data послыает данные в последовательности:
  // flags, name, manu. поэтому используем esp_ble_gap_config_adv_data_raw

  struct {
    BleAdvField<uint8_t, ESP_BLE_AD_TYPE_FLAG> flags;
    BleAdvField<TionAdvManuData, ESP_BLE_AD_MANUFACTURER_SPECIFIC_TYPE> manu;
    BleAdvField<char[sizeof(BLE_SERVICE_NAME) - 1], ESP_BLE_AD_TYPE_NAME_CMPL> name;
  } PACKED raw_adv_data{
      .flags{ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT},
      .manu{TionAdvManuData{
          .vnd_id = 0xFFFF,
          .mac = {},
          .type = tion_dev_info_t::device_type_t::BR4S,
          .pair = pair,
      }},
      .name{BLE_SERVICE_NAME},
  };

  static_assert(sizeof(raw_adv_data) == 30);
  esp_ble_gap_config_adv_data_raw(reinterpret_cast<uint8_t *>(&raw_adv_data), sizeof(raw_adv_data));
}

void Tion4sRC::on_frame(uint16_t type, const uint8_t *data, size_t size) {
  switch (type) {
    case FRAME_TYPE_STATE_REQ: {
      using tion4s_raw_state_t = tion4s_raw_frame_t<tion4s_state_t>;
      ESP_LOGV(TAG, "State GET %s", format_hex_pretty(data, size).c_str());
      const auto *get = reinterpret_cast<const tion4s_raw_state_t *>(data);
      TION_RC_4S_DUMP(TAG, "GET[%u]", get->request_id);
      this->state_req_id_ = get->request_id;
      this->api_->request_state();
      break;
    }

    case FRAME_TYPE_STATE_SAV:
    case FRAME_TYPE_STATE_SET: {
      using tion4s_raw_state_set_t = tion4s_raw_frame_t<tion4s_state_set_t>;
      ESP_LOGV(TAG, "State SET: %s", format_hex_pretty(data, size).c_str());

      const auto *set = reinterpret_cast<const tion4s_raw_state_set_t *>(data);
      this->state_req_id_ = set->request_id;

      const TionGatePosition gate =                                         //-//
          set->data.gate_position == tion4s_state_t::GATE_POSITION_OUTDOOR  //-//
              ? TionGatePosition::OUTDOOR
              : set->data.gate_position == tion4s_state_t::GATE_POSITION_INDOOR  //-//
                    ? TionGatePosition::INDOOR
                    : TionGatePosition::OUTDOOR;

      const bool heat = set->data.heater_mode == dentra::tion_4s::tion4s_state_t::HEATER_MODE_HEATING;

      TION_RC_4S_DUMP(TAG, "SET[%u]", set->request_id);
      TION_RC_4S_DUMP(TAG, "  fan : %u", set->data.fan_speed);
      TION_RC_4S_DUMP(TAG, "  temp: %d", set->data.target_temperature);
      TION_RC_4S_DUMP(TAG, "  flow: %u", static_cast<uint8_t>(gate));
      TION_RC_4S_DUMP(TAG, "  heat: %s", ONOFF(heat));
      TION_RC_4S_DUMP(TAG, "  pwr : %s", ONOFF(set->data.power_state));
      TION_RC_4S_DUMP(TAG, "  snd : %s", ONOFF(set->data.sound_state));
      TION_RC_4S_DUMP(TAG, "  led : %s", ONOFF(set->data.led_state));

      TionStateCall call(this->api_);

      // Tion Remote при выключении звука выставляет 0 скорость
      if (set->data.fan_speed > 0) {
        call.set_fan_speed(set->data.fan_speed);
      }
      if (heat || set->data.target_temperature != 0) {
        // Tion Remote при выключенном обогреве всегда отдает 0
        call.set_target_temperature(set->data.target_temperature);
      }
      call.set_gate_position(gate);
      call.set_heater_state(heat);
      call.set_power_state(set->data.power_state);
      call.set_sound_state(set->data.sound_state);
      call.set_led_state(set->data.led_state);
      call.perform();

      break;
    }

    case FRAME_TYPE_DEV_INFO_REQ: {
      const tion_dev_info_t dev_info{
          .work_mode = tion_dev_info_t::NORMAL,
          .device_type = tion_dev_info_t::BR4S,
          .firmware_version = this->api_->get_state().firmware_version,
          .hardware_version = this->api_->get_state().hardware_version,
          .reserved = {},
      };
      this->pr_.write_frame(FRAME_TYPE_DEV_INFO_RSP, &dev_info, sizeof(dev_info));
      break;
    }

    case FRAME_TYPE_TURBO_REQ: {
      using tion4s_raw_turbo_rsp_t = tion4s_raw_frame_t<tion4s_turbo_t>;
      const tion4s_raw_turbo_rsp_t rsp{
          .request_id = *reinterpret_cast<const uint32_t *>(data),
          .data =
              {
                  .is_active = this->api_->get_state().boost_time_left > 0,
                  .turbo_time = this->api_->get_state().boost_time_left,
                  .err_code = 0,
              },
      };
      this->pr_.write_frame(FRAME_TYPE_TURBO_RSP, &rsp, sizeof(rsp));
      break;
    }

    case FRAME_TYPE_TURBO_SET: {
      using tion4s_raw_turbo_req_t = tion4s_raw_frame_t<tion4s_turbo_set_t>;
      using tion4s_raw_turbo_rsp_t = tion4s_raw_frame_t<tion4s_turbo_t>;

      const auto *set = reinterpret_cast<const tion4s_raw_turbo_req_t *>(data);

      TionStateCall call(this->api_);
      this->api_->enable_boost(set->data.time, &call);
      call.perform();

      const tion4s_raw_turbo_rsp_t rsp{
          .request_id = set->request_id,
          .data = {.is_active = set->data.time > 0, .turbo_time = set->data.time, .err_code = 0}};
      this->pr_.write_frame(FRAME_TYPE_TURBO_RSP, &rsp, sizeof(rsp));
      break;
    }

    case FRAME_TYPE_TIME_REQ: {
      using tion4s_raw_time_rsp_t = tion4s_raw_frame_t<tion4s_time_t>;
      const uint32_t request_id = size == sizeof(uint32_t) ? *reinterpret_cast<const uint32_t *>(data) : 0UL;
      const tion4s_raw_time_rsp_t rsp{.request_id = request_id, .data = {.unix_time = 0}};
      this->pr_.write_frame(FRAME_TYPE_TIME_RSP, &rsp, sizeof(rsp));
      break;
    }

    case FRAME_TYPE_TIMER_REQ: {
      using tion4s_raw_timer_state_req_t = tion4s_raw_frame_t<tion4s_timer_req_t>;
      using tion4s_raw_timer_state_rsp_t = tion4s_raw_frame_t<tion4s_timer_rsp_t>;
      const auto *req = reinterpret_cast<const tion4s_raw_timer_state_req_t *>(data);
      const tion4s_raw_timer_state_rsp_t rsp{
          .request_id = req->request_id,
          .data =
              {
                  .timer_id = req->data.timer_id,
                  .timer = {},
              },
      };
      this->pr_.write_frame(FRAME_TYPE_TIMER_RSP, &rsp, sizeof(rsp));
      break;
    }

    default:
      ESP_LOGW(TAG, "Unknown packet type %04X: %s", type, format_hex_pretty(data, size).c_str());
      break;
  }
}

void Tion4sRC::on_state(const TionState &st) {
  dentra::tion_4s::tion4s_raw_frame_t<dentra::tion_4s::tion4s_state_t> state{};

  state.request_id = this->state_req_id_;

  state.data.fan_speed = st.fan_speed;
  state.data.outdoor_temperature = st.outdoor_temperature;
  state.data.target_temperature = st.target_temperature;
  state.data.power_state = st.power_state;
  state.data.heater_mode = st.heater_state ? tion4s_state_t::HEATER_MODE_HEATING : tion4s_state_t::HEATER_MODE_FANONLY;
  state.data.filter_state = st.filter_state;
  state.data.sound_state = st.sound_state;
  state.data.led_state = st.led_state;
  state.data.current_temperature = st.current_temperature;
  state.data.counters.filter_time = st.filter_time_left;
  state.data.gate_position =                         //-//
      st.gate_position == TionGatePosition::OUTDOOR  //-//
          ? tion4s_state_t::GATE_POSITION_OUTDOOR
          : st.gate_position == TionGatePosition::INDOOR  //-//
                ? tion4s_state_t::GATE_POSITION_INDOOR
                : tion4s_state_t::GATE_POSITION_OUTDOOR;

  // пульту обязательно требуется знать максимальную скорость
  state.data.max_fan_speed = this->api_->get_traits().max_fan_speed;
  state.data.heater_var = st.heater_var;

  TION_RC_4S_DUMP(TAG, "RSP[%u]", state.request_id);
  TION_RC_4S_DUMP(TAG, "  fan : %u", st.fan_speed);
  TION_RC_4S_DUMP(TAG, "  temp: %d", st.target_temperature);
  TION_RC_4S_DUMP(TAG, "  flow: %u", static_cast<uint8_t>(st.gate_position));
  TION_RC_4S_DUMP(TAG, "  heat: %s", ONOFF(st.heater_state));
  TION_RC_4S_DUMP(TAG, "  pwr : %s", ONOFF(st.power_state));
  TION_RC_4S_DUMP(TAG, "  snd : %s", ONOFF(st.sound_state));

  this->pr_.write_frame(FRAME_TYPE_STATE_RSP, &state, sizeof(state));
  this->state_req_id_ = 0;
}

}  // namespace tion_rc
}  // namespace esphome
