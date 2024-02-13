#include "log.h"
#include "utils.h"
#include "tion-api-o2.h"
#include "tion-api-o2-internal.h"

/*
21.01.2023 20:58 (внешнаяя температура в мск T=-10 (0xF6) T=-12 (0xF4) Td=-13 (0xF3))
Через 1.138 сек. после запуска
    RF модуль: 00 ff
    Бризер: 10 04 10 01 00 FA
    RF модуль: 07 F8
    Бризер: 17 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 08 61 0E 13 04 10 EC 19 79
    RF модуль: 01 FE
            00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18
    Бризер: 11 0С 14 17 10 02 3С 04 00 00 04 0А ВС 01 47 73 СВ 01 E3

Через ~200 миллисек и каждые 200 миллисек.
    RF модуль: 03 FC
    Бризер: 13 00 EC
    RF модуль: 04 00 FB
    Бризер: 55 AA

22.01.2023 23:07 (внешнаяя температура в мск T=-10 (0xF6))
## Если RF модуль не подключён к бризеру

    0. pause ~1137 msec
    1. RF модуль: 00 FF
    2. pause ~10 msec
    3. RF модуль: 00 FF
    4. pause ~10 msec
    5. RF модуль: 00 FF
    6. pause ~210 msec
    7. GOTO 1

## Если RF модуль подключен к бризеру
0.  pause ~1138 msec
1.  RF модуль: 00 FF
2.  Бризер: 10 04 10 01 00 FA
3.  RF модуль: 07 F8
4.  Бризер: 17 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 08 61 0E 13 04 10 EC 19 79
5.  RF модуль: 01 FE
            00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18
6.  Бризер: 11 0C 0C 13 10 02 3C 04 00 00 B4 D6 DC 01 01 F6 CA 01 54
            11 0C 0F 15 10 02 3C 04 00 00 B4 D6 DC 01 01 F6 CA 01 51
            11 0C 11 16 10 02 3C 04 00 00 B4 D6 DC 01 01 F6 CA 01 4C
            11 0C FF 0D 10 04 78 04 00 00 F0 D6 DC 01 89 F5 CA 01 34 (температура 0 или 1, нагрев 16)
            11 0C FD 10 10 04 78 04 00 00 2C D7 DC 01 11 F5 CA 01 6E (температура -3 или -4, нагрев 16)
            11 0C FE 0F 10 02 3C 04 00 00 68 D7 DC 01 99 F4 CA 01 FD (температура -2 или -3, нагрев 16)
            11 0C FE 0D 0A 02 3C 04 00 00 E0 D7 DC 01 21 F4 CA 01 D5 (температура -2 или -3, нагрев 10)
7.  pause ~200 msec
8.  RF модуль: 03 FC
9.  Бризер: 13 00 EC
10. RF модуль: 04 00 FB
11. Бризер: 55 AA
12. GOTO 7
*/

namespace dentra {
namespace tion_o2 {

using namespace tion;

static const char *const TAG = "tion-api-uart-o2";

size_t get_req_frame_size(uint8_t frame_type) {
  if (frame_type == FRAME_TYPE_DEV_MODE_REQ) {
    return 1;
  }
  if (frame_type == FRAME_TYPE_SET_WORK_MODE_REQ) {
    return 2;
  }
  if (frame_type == FRAME_TYPE_STATE_GET_REQ) {
    return 1;
  }
  if (frame_type == FRAME_TYPE_STATE_SET_REQ) {
    // 02 01 EC 01 01 01 11
    return 6;
  }
  if (frame_type == FRAME_TYPE_DEV_INFO_REQ) {
    return 1;
  }
  if (frame_type == FRAME_TYPE_CONNECT_REQ) {
    return 1;
  }
  if (frame_type == FRAME_TYPE_TIME_GET_REQ) {
    return 1;
  }
  if (frame_type == FRAME_TYPE_TIME_SET_REQ) {
    // 06 16 34 09 D2
    return 3;
  }
  return 0;
}

size_t get_rsp_frame_size(uint8_t frame_type) {
  if (frame_type == FRAME_TYPE_DEV_MODE_RSP) {
    // 13 00 EC
    return 2;
  }
  if (frame_type == FRAME_TYPE_SET_WORK_MODE_RSP) {
    // 55 AA
    return 1;
  }
  if (frame_type == FRAME_TYPE_STATE_GET_RSP) {
    // 11 0C FE 0D 0A 02 3C 04
    // 00 00 E0 D7 DC 01 21 F4
    // CA 01 D5
    return 18;
  }
  if (frame_type == FRAME_TYPE_DEV_INFO_RSP) {
    // 17 04 00 00 00 00 00 00
    // 00 00 00 00 00 00 00 00
    // 00 08 61 0E 13 04 10 EC
    // 19 79
    return 25;
  }
  if (frame_type == FRAME_TYPE_CONNECT_RSP) {
    // 10 04 10 01 00 FA
    return 5;
  }
  if (frame_type == FRAME_TYPE_TIME_GET_RSP) {
    // 15 16 34 09 C1
    return 4;
  }
  return 0;
}

void tiono2_state_t::for_each_error(const std::function<void(uint8_t error, const char type[3])> &fn) const {
  if (errors == 0) {
    return;
  }
  for (uint32_t i = tiono2_state_t::ERROR_MIN_BIT; i <= tiono2_state_t::ERROR_MAX_BIT; i++) {
    uint32_t mask = 1 << i;
    if ((errors & mask) == mask) {
      fn(i + 1, "EC");
    }
  }
}

void TionO2Api::read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
  if (frame_type == FRAME_TYPE_STATE_GET_RSP) {
    TION_LOGD(TAG, "Response[] State Get");
    if (this->on_state) {
      this->on_state(*static_cast<const tiono2_state_t *>(frame_data), 0);
    }
    return;
  }

  if (frame_type == FRAME_TYPE_DEV_MODE_RSP) {
    // 13 00 EC
    struct RawDevModeFrame {
      union {
        DevModeFlags dev_mode;
        uint8_t data;
      };
    } PACKED;
    auto *frame = static_cast<const RawDevModeFrame *>(frame_data);
    TION_LOGD(TAG, "Response[] Dev mode: %02X", frame->data);
    // if (this->on_dev_mode) {
    //   this->on_dev_mode(frame->dev_mode);
    // }
    return;
  }

  if (frame_type == FRAME_TYPE_SET_WORK_MODE_RSP) {
    // 55 AA
    TION_LOGD(TAG, "Response[] Work Mode");
    return;
  }

  if (frame_type == FRAME_TYPE_DEV_INFO_RSP) {
    // 17 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 08 61 0E 13 04 10 EC 19 79
    TION_LOGD(TAG, "Response[] Device info: %s", hexencode(frame_data, frame_data_size).c_str());
    if (this->on_dev_info) {
      auto *data = static_cast<const tiono2_dev_info_t *>(frame_data);
      tion::tion_dev_info_t info{
          .work_mode = tion::tion_dev_info_t::UNKNOWN,
          .device_type = tion::tion_dev_info_t::BRO2,
          .firmware_version = data->firmware_version,
          .hardware_version = data->hardware_version,
          .reserved = {},
      };
      this->on_dev_info(info);
    }
    return;
  }

  if (frame_type == FRAME_TYPE_CONNECT_RSP) {
    // 10 04 10 01 00 FA
    TION_LOGD(TAG, "Response[] Connect: %s", hexencode(frame_data, frame_data_size).c_str());
    return;
  }

  if (frame_type == FRAME_TYPE_TIME_GET_RSP) {
    // 15 0B 09 1A F2
    auto *data = static_cast<const tiono2_time_t *>(frame_data);
    TION_LOGD(TAG, "Response[] Time: %02u:%02u:%02u", data->hours, data->minutes, data->seconds);
    return;
  }

  TION_LOGW(TAG, "Unsupported frame %02X: %s", frame_type, hexencode(frame_data, frame_data_size).c_str());
}

bool TionO2Api::request_connect() const {
  TION_LOGD(TAG, "Request[] Connect");
  return this->write_frame(FRAME_TYPE_CONNECT_REQ);
}

bool TionO2Api::request_dev_info() const {
  TION_LOGD(TAG, "Request[] Device info");
  return this->write_frame(FRAME_TYPE_DEV_INFO_REQ);
}

bool TionO2Api::request_state() const {
  TION_LOGD(TAG, "Request[] State Get");
  return this->write_frame(FRAME_TYPE_STATE_GET_REQ);
}

bool TionO2Api::request_dev_mode() const {
  TION_LOGV(TAG, "Request[] Dev mode");
  return this->write_frame(FRAME_TYPE_DEV_MODE_REQ);
}

bool TionO2Api::set_work_mode(WorkModeFlags work_mode) const {
  TION_LOGV(TAG, "Request[] Work mode");
  const struct {
    WorkModeFlags work_mode;
  } PACKED data{.work_mode = work_mode};
  return this->write_frame(FRAME_TYPE_SET_WORK_MODE_REQ, data);
}

bool TionO2Api::write_state(const tiono2_state_t &state, uint32_t request_id) const {
  TION_LOGV(TAG, "Request[] State set");
  tiono2_state_set_t data{
      .fan_speed = state.fan_speed,
      .target_temperature = state.target_temperature,
      .power = state.power_state,
      .heat = state.heater_state,
      .source = tiono2_state_set_t::USER,
  };
  return this->write_frame(FRAME_TYPE_STATE_SET_REQ, data);
}

bool TionO2Api::reset_filter(const tiono2_state_t &state, uint32_t request_id) const { return false; }

}  // namespace tion_o2
}  // namespace dentra
