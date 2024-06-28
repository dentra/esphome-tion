#include <cinttypes>
#include "esphome/core/defines.h"
#include "esphome/core/log.h"

#include "../tion-api/tion-api-3s-internal.h"

#include "tion_rc_3s.h"

namespace esphome {
namespace tion_rc {

static const char *const TAG = "tion_rc_3s";

using namespace dentra::tion;
using namespace dentra::tion_3s;

#define BLE_SERVICE_NAME "Tion Breezer 3S"

void Tion3sRC::adv(bool pair) {
  esp_ble_gap_set_device_name(BLE_SERVICE_NAME);
  esp_ble_adv_data_t adv_data{};
  adv_data.include_name = true;
  if (pair) {
    adv_data.flag = ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT;
  }
  esp_ble_gap_config_adv_data(&adv_data);
}

void Tion3sRC::on_frame(uint16_t type, const uint8_t *data, size_t size) {
  switch (type) {
    case FRAME_TYPE_REQ(FRAME_TYPE_STATE_GET): {
      this->state_req_id_ = 1;
      this->api_->request_state();
      break;
    }

    case FRAME_TYPE_REQ(FRAME_TYPE_STATE_SET): {
      this->state_req_id_ = 1;
      const auto *set = reinterpret_cast<const tion3s_state_set_t *>(data);

      const TionGatePosition gate =                                    //-//
          set->gate_position == tion3s_state_t::GATE_POSITION_OUTDOOR  //-//
              ? TionGatePosition::OUTDOOR
              : set->gate_position == tion3s_state_t::GATE_POSITION_INDOOR  //-//
                    ? TionGatePosition::INDOOR
                    : set->gate_position == tion3s_state_t::GATE_POSITION_MIXED  //-//
                          ? TionGatePosition::MIXED
                          : TionGatePosition::OUTDOOR;

      TION_RC_DUMP(TAG, "SET");
      TION_RC_DUMP(TAG, "  fan : %u", set->fan_speed);
      TION_RC_DUMP(TAG, "  temp: %d", set->target_temperature);
      TION_RC_DUMP(TAG, "  flow: %u", static_cast<uint8_t>(gate));
      TION_RC_DUMP(TAG, "  heat: %s", ONOFF(set->flags.heater_state));
      TION_RC_DUMP(TAG, "  pwr : %s", ONOFF(set->flags.power_state));
      TION_RC_DUMP(TAG, "  snd : %s", ONOFF(set->flags.sound_state));

      dentra::tion::TionStateCall call(this->api_);

      // Tion Remote при выключении звука выставляет 0 скорость
      if (set->fan_speed > 0) {
        call.set_fan_speed(set->fan_speed);
      }
      if (set->flags.heater_state || set->target_temperature != 0) {
        // Tion Remote при выключенном обогреве всегда отдает 0
        call.set_target_temperature(set->target_temperature);
      }
      call.set_gate_position(gate);
      call.set_heater_state(set->flags.heater_state);
      call.set_power_state(set->flags.power_state);
      call.set_sound_state(set->flags.sound_state);
      call.perform();

      break;
    }

    default:
      ESP_LOGW(TAG, "Unknown packet type %04X: %s", type, format_hex_pretty(data, size).c_str());
      break;
  }
}

void Tion3sRC::on_state(const TionState &st) {
  tion3s_state_t state{};

  state.fan_speed = st.fan_speed;
  state.outdoor_temperature = st.outdoor_temperature;
  state.target_temperature = st.target_temperature;
  state.flags.power_state = st.power_state;
  state.flags.heater_state = st.heater_state;
  state.flags.sound_state = st.sound_state;
  state.current_temperature1 = st.current_temperature;
  state.current_temperature2 = st.current_temperature;
  state.gate_position =                              //-//
      st.gate_position == TionGatePosition::OUTDOOR  //-//
          ? tion3s_state_t::GATE_POSITION_OUTDOOR
          : st.gate_position == TionGatePosition::INDOOR  //-//
                ? tion3s_state_t::GATE_POSITION_INDOOR
                : st.gate_position == TionGatePosition::MIXED  //-//
                      ? tion3s_state_t::GATE_POSITION_MIXED
                      : tion3s_state_t::GATE_POSITION_OUTDOOR;
  state.filter_time = st.filter_time_left / (3600 * 24);
  state.firmware_version = st.firmware_version;
  state.productivity = st.productivity;

  TION_RC_DUMP(TAG, "RSP");
  TION_RC_DUMP(TAG, "  fan : %u", st.fan_speed);
  TION_RC_DUMP(TAG, "  temp: %d", st.target_temperature);
  TION_RC_DUMP(TAG, "  flow: %u", static_cast<uint8_t>(st.gate_position));
  TION_RC_DUMP(TAG, "  heat: %s", ONOFF(st.heater_state));
  TION_RC_DUMP(TAG, "  pwr : %s", ONOFF(st.power_state));
  TION_RC_DUMP(TAG, "  snd : %s", ONOFF(st.sound_state));

  this->pr_.write_frame(FRAME_TYPE_RSP(FRAME_TYPE_STATE_GET), &state, sizeof(state));

  this->state_req_id_ = 0;
}

}  // namespace tion_rc
}  // namespace esphome
