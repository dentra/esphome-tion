#pragma once

#include "tion-api.h"

namespace dentra {
namespace tion_o2 {

#pragma pack(push, 1)
// 11 0C FE 0D 0A 02 3C 04
// 00 00 E0 D7 DC 01 21 F4
// CA 01 D5
// Ответ 11:
// Байт 0 - команда 11
// Байт 2 - температура
// Байт 4 - нагрев
// NOLINTNEXTLINE(readability-identifier-naming)
struct tiono2_state_t {
  // Байт 1.
  struct {
    uint8_t unknown : 1;
    bool power_state : 1;
    bool sound_state : 1;
    bool heater_state : 1;
    uint8_t reserved : 4;
  } flags;
  // Байт 2. температура
  int8_t outdoor_temperature;
  // Байт 3.
  int8_t current_temperature;
  // Байт 4. Целевая температура [-20:25].
  int8_t target_temperature;
  // Байт 5. Скорость вентиляции [1-4].
  uint8_t fan_speed;
  // Байт 6. Производительность
  uint8_t productivity;
  // Байт 7.
  uint8_t unknown7;  // 04
  // Байт 8.
  uint8_t unknown8;  // 00
  // Байт 9.
  uint8_t unknown9;  // 00
  // Байт 10,11,12,13
  struct {
    // Время наработки в секундах.
    uint32_t work_time;
    uint32_t filter_time;
    uint32_t work_time_days() const { return this->work_time / (24 * 3600); }
    uint32_t filter_time_left() const { return this->filter_time / (24 * 3600); }
  } counters;

  bool filter_warnout() const { return this->counters.filter_time <= 30; }

  // FIXME сейчас первое попавшееся значение
  bool is_initialized() const { return this->counters.work_time != 0; }

  bool is_heating() const {
    if (!this->flags.heater_state) {
      return false;
    }
    // heating detection borrowed from:
    // https://github.com/TionAPI/tion_python/blob/master/tion_btle/tion.py#L177
    // self.heater_temp - self.in_temp > 3 and self.out_temp > self.in_temp
    return (this->target_temperature - this->outdoor_temperature) > 3 &&
           (this->current_temperature > this->outdoor_temperature);
  }
};

static_assert(sizeof(tiono2_state_t) == 17, "Invalid tiono2_state_t size");

struct tiono2_time_t {
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
};

static_assert(sizeof(tiono2_time_t) == 3, "Invalid tiono2_time_t size");

struct tiono2_state_set_t {
  // Байт 1. Скорость вентиляции [1:4].
  uint8_t fan_speed;
  // Байт 2. Целевая температура [-20:25].
  int8_t target_temperature;
  // Байт 3., 0 off, 1 on 4
  struct {
    bool power : 8;
  };
  // Байт 4.
  struct {
    bool heat : 8;
  };
  // Байт 5. Иозможно источник:  0 - ma_auto, 1 - app
  enum : uint8_t { MA = 0, APP = 1 } source;
};

static_assert(sizeof(tiono2_state_set_t) == 5, "Invalid tiono2_state_set_t size");

// 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25
// 17 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 08 61 0E 13 04 10 EC 19 79
struct tiono2_dev_info_t {
  // Байт 1.
  uint8_t unknown1;
  // Байт 2-16. заполнено 00
  uint8_t unknown2_16[15];
  // Байт 17.
  uint8_t unknown17;
  // Байт 18.
  uint8_t unknown18;
  // Байт 19.
  uint8_t unknown19;
  // Байт 20.
  uint8_t unknown20;
  // Байт 21,22. Версия прошивки бризера.
  uint16_t firmware_version;
  // Байт 23.
  uint8_t unknown23;
  // Байт 24.
  uint8_t unknown24;
};

static_assert(sizeof(tiono2_dev_info_t) == 24, "Invalid tiono2_dev_info_t size");

#pragma pack(pop)

// returns response frame size including crc
size_t get_req_frame_size(uint8_t frame_type);

// returns response frame size including crc
size_t get_rsp_frame_size(uint8_t frame_type);

class TionO2Api : public tion::TionApiBase<tiono2_state_t> {
 public:
  void read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size);

  uint16_t get_state_type() const;

  bool request_connect() const;
  bool request_dev_info() const;
  bool request_state() const;

  bool write_state(const tiono2_state_t &state, uint32_t request_id) const;
  bool reset_filter(const tiono2_state_t &state, uint32_t request_id = 1) const;
  bool factory_reset(const tiono2_state_t &state, uint32_t request_id = 1) const;
  bool reset_errors(const tiono2_state_t &state, uint32_t request_id = 1) const;

  bool send_heartbeat() const;
  bool send_work_mode(uint8_t work_mode = 0) const;
};

}  // namespace tion_o2
}  // namespace dentra
