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

struct tion3s_state_t {
  enum GatePosition : uint8_t { GATE_POSITION_INDOOR = 0, GATE_POSITION_MIXED = 1, GATE_POSITION_OUTDOOR = 2 };
  uint8_t fan_speed : 4;
  GatePosition gate_position : 4;
  // настроенная температура подогрева
  int8_t target_temperature;
  struct Flags {
    bool heater_state : 1;
    bool power_state : 1;
    bool timer_state : 1;
    bool sound_state : 1;
    bool auto_state : 1;
    bool ma_connect : 1;
    bool save : 1;
    bool ma_pairing : 1;
    bool preset_state : 1;
    uint8_t reserved : 7;
  } flags;
  int8_t unknown_temperature;
  // текущая температура воздуха после нагревателя (т.е. текущая температура внутри помещения);
  int8_t current_temperature;
  // температура воздуха на входе в бризер (т.е. текущая температура на улице)
  int8_t outdoor_temperature;
  struct {
    // остаточный ресурс фильтров в днях (то, что показывается в приложении как "время жизни фильтров")
    uint16_t filter_time;
    uint16_t filter_time_left() const { return filter_time; }
  } counters;
  uint8_t hours;
  uint8_t minutes;
  uint8_t last_error;
  // текущая производительность бризера в кубометрах в час
  uint8_t productivity;
  // сколько дней прошло с момента установки новых фильтров
  uint16_t filter_days;
  // текущая версия прошивки
  uint16_t firmware_version;

  bool is_initialized() const { return this->firmware_version != 0; }
  bool filter_warnout() const { return this->counters.filter_time <= 10; }
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
  bool write_state(const tion3s_state_t &state) const;
  bool reset_filter(const tion3s_state_t &state) const;

#ifdef TION_ENABLE_HEARTBEAT
  bool send_heartbeat() const { return false; }
#endif
};

}  // namespace tion
}  // namespace dentra
