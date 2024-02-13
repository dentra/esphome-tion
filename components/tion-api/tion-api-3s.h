#pragma once

#include "tion-api.h"

namespace dentra {
namespace tion {

#pragma pack(push, 1)

enum : uint8_t {
  FRAME_MAGIC_REQ = 0x3D,
  FRAME_MAGIC_RSP = 0xB3,
  FRAME_MAGIC_END = 0x5A,
};

// NOLINTNEXTLINE(readability-identifier-naming)
struct tion3s_state_t {
  enum GatePosition : uint8_t { GATE_POSITION_INDOOR = 0, GATE_POSITION_MIXED = 1, GATE_POSITION_OUTDOOR = 2 };
  // Байт 0, бит 0-3. Скорость вентиляции.
  uint8_t fan_speed : 4;
  // Байт 0, бит 4-7. Позиция заслонки.
  GatePosition gate_position : 4;
  // Байт 1. Настроенная температура подогрева.
  int8_t target_temperature;
  // Байт 2-3. Флаги.
  struct Flags {
    // Байт 2, бит 0. Включен режим подогрева.
    bool heater_state : 1;
    // Байт 2, бит 1. Включен бризер.
    bool power_state : 1;
    // Байт 2, бит 2. Включен таймер.
    bool timer_state : 1;
    // Байт 2, бит 3. Включены звуковые оповещения.
    bool sound_state : 1;
    // Байт 2, бит 4.
    bool auto_state : 1;
    // Байт 2, бит 5. Состояния подключения MagicAir.
    bool ma_connect : 1;
    // Байт 2, бит 6.
    bool save : 1;
    // Байт 2, бит 7.
    bool ma_pairing : 1;
    // Байт 3, бит 0.
    bool preset_state : 1;
    // Байт 3, бит 1.
    bool presets_state : 1;
    // зарезервированно
    uint8_t reserved : 6;
  } flags;
  // Байт 4.
  int8_t current_temperature1;
  // Байт 5. Текущая температура воздуха после нагревателя (внутри помещения).
  int8_t current_temperature2;
  // Байт 6. Температура воздуха на входе в бризер (температура на улице).
  int8_t outdoor_temperature;
  // Байт 7-8.
  struct {
    // Остаточный ресурс фильтров в днях. То, что показывается в приложении как "время жизни фильтров".
    uint16_t filter_time;
    uint16_t filter_time_left() const { return this->filter_time > 360 ? 1 : this->filter_time; }
  } counters;
  // Байт 9. Часы.
  uint8_t hours;
  // Байт 10. Минуты.
  uint8_t minutes;
  // Байт 11. Код последней ошибки. Выводиться в формате "EC%u".
  uint8_t last_error;
  // Байт 12. Текущая производительность бризера в кубометрах в час.
  uint8_t productivity;
  // Байт 13. Сколько дней прошло с момента установки новых фильтров.
  uint16_t filter_days;
  // Байт 15-16. Текущая версия прошивки.
  uint16_t firmware_version;

  int current_temperature() const {
    // Исходная формула:
    // ((bArr[4] <= 0 ? bArr[5] : bArr[4]) + (bArr[5] <= 0 ? bArr[4] : bArr[5])) / 2;
    auto barr_4 = this->current_temperature1;
    auto barr_5 = this->current_temperature2;
    return ((barr_4 <= 0 ? barr_5 : barr_4) + (barr_5 <= 0 ? barr_4 : barr_5)) / 2;
  }

  bool is_initialized() const { return this->firmware_version != 0; }
  bool filter_warnout() const { return this->counters.filter_time <= 10; }

  bool is_heating() const {
    if (!this->flags.heater_state) {
      return false;
    }
    // heating detection borrowed from:
    // https://github.com/TionAPI/tion_python/blob/master/tion_btle/tion.py#L177
    // self.heater_temp - self.in_temp > 3 and self.out_temp > self.in_temp
    return (this->target_temperature - this->outdoor_temperature) > 3 &&
           (this->current_temperature() > this->outdoor_temperature);
  }

  void for_each_error(const std::function<void(uint8_t error, const char type[3])> &fn) const;
};

#pragma pack(pop)

class Tion3sApi : public TionApiBase<tion3s_state_t> {
 public:
  void read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size);

  uint16_t get_state_type() const;

  bool request_dev_info() const { return false; }

  bool pair() const;
  bool request_state() const;
  bool request_command4() const;
  bool write_state(const tion3s_state_t &state, uint32_t unused_request_id) const;
  bool reset_filter(const tion3s_state_t &state) const;
  bool factory_reset(const tion3s_state_t &state) const;

#ifdef TION_ENABLE_HEARTBEAT
  bool send_heartbeat() const { return false; }
#endif
};

}  // namespace tion
}  // namespace dentra
