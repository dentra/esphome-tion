#include <cinttypes>
#include "esphome/core/defines.h"
#include "esphome/core/log.h"

#include "../tion-api/tion-api-4s-internal.h"

#include "tion_rc_4s.h"

#ifdef TION_UPDATE_EMU
#include "../tion-api/tion-api-firmware.h"
#include "../tion-api/crc.h"
#endif

namespace esphome {
namespace tion_rc {

static const char *const TAG = "tion_rc_4s";

using namespace dentra::tion;
using namespace dentra::tion_4s;
#ifdef TION_UPDATE_EMU
using namespace dentra::tion::firmware;
#endif

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
  // #ifndef __clang__
  //   BleAdvField(const char *data) { memcpy(this->data, data, sizeof(T)); }
  // #endif
} PACKED;

void Tion4sRC::adv(bool pair) {
  // Для 4S advertising содержит следующую информацию:
  // Bluetooth HCI Event - LE Meta
  //     Event Code: LE Meta (0x3e)
  //     Parameter Total Length: 42
  //     Sub Event: LE Advertising Report (0x02)
  //     Num Reports: 1
  //     Event Type: Connectable Undirected Advertising (0x00)
  //     Peer Address Type: Random Device Address (0x01)
  //     BD_ADDR: 11:22:33:44:55:66 (11:22:33:44:55:66)
  //     Data Length: 30
  //     Advertising Data
  //         Flags
  //             Length: 2
  //             Type: Flags (0x01)
  //             000. .... = Reserved: 0x0
  //             ...0 .... = Simultaneous LE and BR/EDR to Same Device Capable (Host): false (0x0)
  //             .... 0... = Simultaneous LE and BR/EDR to Same Device Capable (Controller): false (0x0)
  //             .... .1.. = BR/EDR Not Supported: true (0x1)
  //             .... ..1. = LE General Discoverable Mode: true (0x1)
  //             .... ...0 = LE Limited Discoverable Mode: false (0x0)
  //         Manufacturer Specific
  //             Length: 14
  //             Type: Manufacturer Specific (0xff)
  //             Company ID: For use in internal and interoperability tests (0xffff)
  //             Data: 6655443322110380000001
  //         Device Name: Breezer 4S
  //             Length: 11
  //             Type: Device Name (0x09)
  //             Device Name: Breezer 4S
  //     RSSI: -56 dBm

  esp_ble_gap_set_device_name(BLE_SERVICE_NAME);

  struct TionAdvManuData {
    uint16_t company_id;
    esp_bd_addr_t mac;
    tion_dev_info_t::device_type_t type;
    struct {
      bool pair : 8;
    };
  } PACKED;
  static_assert(sizeof(TionAdvManuData) == 13);

  // для пульта важна последовательность следования данных внутри "рекламы", требуется:
  // flags, manu, name. esp_ble_gap_config_adv_data посылает данные в последовательности:
  // flags, name, manu. поэтому используем esp_ble_gap_config_adv_data_raw

  struct {
    BleAdvField<uint8_t, ESP_BLE_AD_TYPE_FLAG> flags;
    BleAdvField<TionAdvManuData, ESP_BLE_AD_MANUFACTURER_SPECIFIC_TYPE> manu;
    BleAdvField<char[sizeof(BLE_SERVICE_NAME) - 1], ESP_BLE_AD_TYPE_NAME_CMPL> name;
  } PACKED raw_adv_data{
      .flags{ESP_BLE_ADV_FLAG_BREDR_NOT_SPT},
      .manu{TionAdvManuData{
          .company_id = 0xFFFF,
          .mac = {},
          .type = tion_dev_info_t::device_type_t::BR4S,
          .pair = pair,
      }},
      .name{BLE_SERVICE_NAME},
  };
  if (pair) {
    raw_adv_data.flags.data |= ESP_BLE_ADV_FLAG_GEN_DISC;
  }
  // пульт не проверяет адрес в "рекламе"
  // esp_bd_addr_t mac;
  // esp_read_mac(mac, ESP_MAC_BT);
  // for (int i = 0; i < sizeof(esp_bd_addr_t); i++) {
  //   raw_adv_data.manu.data.mac[i] = mac[(sizeof(esp_bd_addr_t) - 1) - i];
  // }

  static_assert(sizeof(raw_adv_data) == 30);
  esp_ble_gap_config_adv_data_raw(reinterpret_cast<uint8_t *>(&raw_adv_data), sizeof(raw_adv_data));
}

void Tion4sRC::on_frame(uint16_t type, const uint8_t *data, size_t size) {
  switch (type) {
    case FRAME_TYPE_STATE_REQ: {
      using tion4s_raw_state_t = tion4s_state_t;
      ESP_LOGV(TAG, "State GET %s", format_hex_pretty(data, size).c_str());
      const auto *get = reinterpret_cast<const tion4s_raw_state_t *>(data);
      TION_RC_DUMP(TAG, "STATE_GET[]");
      this->state_req_id_ = 1;
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

      TION_RC_DUMP(TAG, "STATE_SET[%" PRIu32 "]", set->request_id);
      TION_RC_DUMP(TAG, "  fan : %u", set->data.fan_speed);
      TION_RC_DUMP(TAG, "  temp: %d", set->data.target_temperature);
      TION_RC_DUMP(TAG, "  flow: %u", static_cast<uint8_t>(gate));
      TION_RC_DUMP(TAG, "  heat: %s", ONOFF(heat));
      TION_RC_DUMP(TAG, "  pwr : %s", ONOFF(set->data.power_state));
      TION_RC_DUMP(TAG, "  snd : %s", ONOFF(set->data.sound_state));
      TION_RC_DUMP(TAG, "  led : %s", ONOFF(set->data.led_state));

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
      TION_RC_DUMP(TAG, "DEV_INFO[]");
      const tion_dev_info_t dev_info{
#ifdef TION_UPDATE_EMU
          .work_mode = this->is_update_ ? tion_dev_info_t::UPDATE : tion_dev_info_t::NORMAL,
#else
          .work_mode = tion_dev_info_t::NORMAL,
#endif
          .device_type = tion_dev_info_t::BR4S,
#ifdef TION_UPDATE_EMU
          .firmware_version = TION_UPDATE_EMU,
          .hardware_version = 1,
#else
          .firmware_version = this->api_->get_state().firmware_version,
          .hardware_version = this->api_->get_state().hardware_version,
#endif
          .reserved = {},
      };
      this->pr_.write_frame(FRAME_TYPE_DEV_INFO_RSP, &dev_info, sizeof(dev_info));
      break;
    }

    case FRAME_TYPE_TURBO_REQ: {
      using tion4s_raw_turbo_rsp_t = tion4s_raw_frame_t<tion4s_turbo_t>;
      const uint32_t request_id = *reinterpret_cast<const uint32_t *>(data);
      TION_RC_DUMP(TAG, "TURBO_GET[%" PRIu32 "]", request_id);
      const tion4s_raw_turbo_rsp_t rsp{
          .request_id = request_id,
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

      TION_RC_DUMP(TAG, "TURBO_SET[%" PRIu32 "]", set->request_id);

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
      TION_RC_DUMP(TAG, "TIME_GET[%" PRIu32 "]", request_id);
      const tion4s_raw_time_rsp_t rsp{.request_id = request_id, .data = {.unix_time = 0}};
      this->pr_.write_frame(FRAME_TYPE_TIME_RSP, &rsp, sizeof(rsp));
      break;
    }

    case FRAME_TYPE_TIMER_REQ: {
      using tion4s_raw_timer_state_req_t = tion4s_raw_frame_t<tion4s_timer_req_t>;
      using tion4s_raw_timer_state_rsp_t = tion4s_raw_frame_t<tion4s_timer_rsp_t>;
      const auto *req = reinterpret_cast<const tion4s_raw_timer_state_req_t *>(data);
      TION_RC_DUMP(TAG, "TIMER_GET[%" PRIu32 "]: timer_id=%u", req->request_id, req->data.timer_id);
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

#ifdef TION_UPDATE_EMU
    case FRAME_TYPE_UPDATE_PREPARE_REQ: {
      ESP_LOGI(TAG, "Entering update mode");

      const FirmwareVersions rsp{
          .device_type = tion_dev_info_t::BR4S,
          .unknown1 = 0,
          .hardware_version = TION_UPDATE_EMU,
      };

      this->is_update_ = true;
      this->fw_size_ = 0;
      this->fw_load_ = 0;
      this->fw_crc_ = 0xFFFF;

      this->pr_.write_frame(FRAME_TYPE_UPDATE_PREPARE_RSP, &rsp, sizeof(rsp));
      break;
    }
    case FRAME_TYPE_UPDATE_START_REQ: {
      if (!this->is_update_) {
        ESP_LOGW(TAG, "Not in update mode");
        this->pr_.write_frame(FRAME_TYPE_UPDATE_ERROR, nullptr, 0);
        break;
      }

      // TODO доп проверка что структура данных верна, перенести в заголовок
      static_assert(sizeof(FirmwareInfo) == 132);

      if (size != sizeof(FirmwareInfo)) {
        ESP_LOGW(TAG, "Invalid update start");
        this->is_update_ = false;
        this->pr_.write_frame(FRAME_TYPE_UPDATE_ERROR, nullptr, 0);
        break;
      }
      const auto *req = reinterpret_cast<const FirmwareInfo *>(data);
      ESP_LOGI(TAG, "Starting update of %" PRIu32 " bytes", req->size);

      this->fw_size_ = req->size - sizeof(FirmwareInfo::data);
      this->pr_.write_frame(FRAME_TYPE_UPDATE_START_RSP, nullptr, 0);
      break;
    }
    case FRAME_TYPE_UPDATE_CHUNK_REQ: {
      if (!this->is_update_) {
        ESP_LOGW(TAG, "Not in update mode");
        this->pr_.write_frame(FRAME_TYPE_UPDATE_ERROR, nullptr, 0);
        break;
      }
      if (this->fw_size_ == 0) {
        ESP_LOGW(TAG, "Invalid update chunk");
        this->is_update_ = false;
        this->pr_.write_frame(FRAME_TYPE_UPDATE_ERROR, nullptr, 0);
        break;
      }
      const auto *req = reinterpret_cast<const FirmwareChunk *>(data);
      const auto chunk_size = size - (sizeof(FirmwareChunk) - sizeof(FirmwareChunk::data));

      if (req->offset == FirmwareChunkCRC::MARKER) {
        if (chunk_size != sizeof(FirmwareChunkCRC::crc)) {
          ESP_LOGW(TAG, "Invalid update final chunk size: %" PRIu32, chunk_size);
          this->is_update_ = false;
          this->pr_.write_frame(FRAME_TYPE_UPDATE_ERROR, nullptr, 0);
          break;
        }
        ESP_LOGI(TAG, "Got final chunk at %" PRIu32, req->offset);
        this->fw_crc_ = crc16_ccitt_false(this->fw_crc_, req->data, chunk_size);
        if (this->fw_crc_ != 0) {
          ESP_LOGW(TAG, "CRC failed");
          this->pr_.write_frame(FRAME_TYPE_UPDATE_ERROR, nullptr, 0);
          break;
        }
      } else {
        if (this->fw_load_ != req->offset) {
          ESP_LOGW(TAG, "Invalid update chunk offset: expected=%" PRIu32 ", actual=%" PRIu32, this->fw_load_,
                   req->offset);
          this->is_update_ = false;
          this->pr_.write_frame(FRAME_TYPE_UPDATE_ERROR, nullptr, 0);
          break;
        }

        ESP_LOGI(TAG, "Got chunk at %" PRIu32, req->offset);
        this->fw_load_ += chunk_size;
        if (this->fw_load_ > this->fw_size_) {
          ESP_LOGW(TAG, "Invalid update chunk size: %" PRIu32, chunk_size);
          this->is_update_ = false;
          this->pr_.write_frame(FRAME_TYPE_UPDATE_ERROR, nullptr, 0);
          break;
        }

        this->fw_crc_ = crc16_ccitt_false(this->fw_crc_, req->data, chunk_size);
      }

      this->pr_.write_frame(FRAME_TYPE_UPDATE_CHUNK_RSP, nullptr, 0);
      break;
    }

    case FRAME_TYPE_UPDATE_FINISH_REQ: {
      if (!this->is_update_) {
        ESP_LOGW(TAG, "Not in update mode");
        this->pr_.write_frame(FRAME_TYPE_UPDATE_ERROR, nullptr, 0);
        break;
      }

      this->is_update_ = false;

      if (size != sizeof(FirmwareInfo)) {
        ESP_LOGW(TAG, "Invalid update finish");
        this->pr_.write_frame(FRAME_TYPE_UPDATE_ERROR, nullptr, 0);
        break;
      }

      const auto *req = reinterpret_cast<const FirmwareInfo *>(data);
      ESP_LOGI(TAG, "Finishing update: %" PRIu32, req->size);

      const auto fw_size = req->size - sizeof(FirmwareInfo::data);
      if (fw_size != this->fw_size_) {
        ESP_LOGW(TAG, "Invalid check size: %" PRIu32, fw_size);
        this->pr_.write_frame(FRAME_TYPE_UPDATE_ERROR, nullptr, 0);
      }

      this->pr_.write_frame(FRAME_TYPE_UPDATE_FINISH_RSP, nullptr, 0);
      break;
    }
#endif

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

  TION_RC_DUMP(TAG, "RSP[%" PRIu32 "]", state.request_id);
  TION_RC_DUMP(TAG, "  fan : %u", st.fan_speed);
  TION_RC_DUMP(TAG, "  temp: %d", st.target_temperature);
  TION_RC_DUMP(TAG, "  flow: %u", static_cast<uint8_t>(st.gate_position));
  TION_RC_DUMP(TAG, "  heat: %s", ONOFF(st.heater_state));
  TION_RC_DUMP(TAG, "  pwr : %s", ONOFF(st.power_state));
  TION_RC_DUMP(TAG, "  snd : %s", ONOFF(st.sound_state));

  this->pr_.write_frame(FRAME_TYPE_STATE_RSP, &state, sizeof(state));
  this->state_req_id_ = 0;
}

}  // namespace tion_rc
}  // namespace esphome
