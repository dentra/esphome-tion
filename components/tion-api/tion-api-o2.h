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
  union {
    struct {
      bool sound_state : 1;
      uint8_t unknown : 7;
    } flags;
    uint8_t unknown1;  // 0C
  };
  // Байт 2. температура
  int8_t outdoor_temperature;
  // Байт 3.
  uint8_t unknown3;  // 0D | 0F | 10 | 16 | 15 | 13 | 17
  // Байт 4. нагрев
  int8_t current_temperature;
  // Байт 5.
  uint8_t unknown5;  // 02 | 04
  // Байт 6.
  uint8_t unknown6;  // 3C | 78
  // Байт 7.
  uint8_t unknown7;  // 04
  // Байт 8.
  uint8_t unknown8;  // 00
  // Байт 9.
  uint8_t unknown9;  // 00
  // Байт 10.
  uint8_t unknown10;
  // Байт 11.
  uint8_t unknown11;  // D7 | D6 | 0A
  // Байт 12.
  uint8_t unknown12;  // DC | BC?
  // Байт 13.
  uint8_t unknown13;  // 01
  // Байт 14.
  uint8_t unknown14;
  // Байт 15.
  uint8_t unknown15;  // F4 | F5 | F6 | 73?
  // Байт 16. Возможно остаток ресурса фильтра, были значения 203, на след день 202
  union {
    uint8_t unknown16;  // CA | CB?
    struct {
      uint8_t unknown;
      uint8_t filter_time_left() const { return 0; }
    } counters;
  };

  // Байт 17.
  uint8_t unknown17;  // 01

  bool filter_warnout() const { return false; }
  // FIXME сейчас первое попавшееся значение
  bool is_initialized() const { return this->unknown17 != 0; }
};
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
