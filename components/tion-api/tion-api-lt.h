#pragma once

#include "tion-api.h"

namespace dentra {
namespace tion {

#pragma pack(push, 1)
// NOLINTNEXTLINE(readability-identifier-naming)
struct tionlt_state_t {
  enum { ERROR_MIN_BIT = 0, ERROR_MAX_BIT = 10, WARNING_MIN_BIT = 24, WARNING_MAX_BIT = 27 };

  enum GateState : uint8_t {
    // закрыто
    CLOSED = 1,
    // открыто
    OPENED = 2,
  };

  // Байт 0-1.
  struct {
    // Байт 0, бит 0. Состояние бризера
    bool power_state : 1;
    // Байт 0, бит 1. Состояние звуковых оповещений
    bool sound_state : 1;
    // Байт 0, бит 2. Состояние световых оповещений
    bool led_state : 1;
    // Байт 0, бит 3.
    uint8_t last_com_source : 1;
    // Байт 0, бит 4. Предупреждение о необходимости замены фильтра.
    bool filter_warnout : 1;
    // Байт 0, бит 5.
    bool auto_co2 : 1;
    // Байт 0, бит 6.
    bool heater_state : 1;
    // Байт 0, бит 7.
    bool heater_present : 1;
    // Байт 1, бит 0. the presence of the KIV mode
    bool kiv_present : 1;
    // Байт 1, бит 1. state of the KIV mode
    bool kiv_active : 1;
    // reserved
    uint8_t reserved : 6;
  } flags;
  GateState gate_state;
  // Байт 3. Настроенная температура подогрева.
  int8_t target_temperature;
  // Байт 4. Скорость вентиляции.
  uint8_t fan_speed;
  // Байт 5. Температура воздуха на входе в бризер (температура на улице).
  int8_t outdoor_temperature;
  // Байт 6. Текущая температура воздуха после нагревателя (внутри помещения).
  int8_t current_temperature;
  // Байт 7. Внутренняя температура платы.
  int8_t pcb_temperature;
  // Байт 8-23. 8-11 - work_time, 12-15 - fan_time, 16-19 - filter_time, 20-23 - airflow_counter
  tion_state_counters_t<tion_dev_info_t::BRLT> counters;
  // Байт 24-47.
  struct {
    uint32_t reg;
    struct {
      enum { ERROR_TYPE_NUMBER = 20 };
      uint8_t er[ERROR_TYPE_NUMBER];
    } cnt;
  } errors;
  // Байт 48-53.
  // NOLINTNEXTLINE(readability-identifier-naming)
  struct button_presets_t {
    enum { PRESET_NUMBER = 3 };
    int8_t temp[PRESET_NUMBER];
    uint8_t fan_speed[PRESET_NUMBER];
  } button_presets;
  // Байт 54.
  uint8_t max_fan_speed;
  // Байт 55.
  uint8_t heater_var;
  // Байт 56.
  uint8_t test_type;
  float heater_power() const;
  bool is_initialized() const { return this->counters.work_time != 0; }
  bool filter_warnout() const { return this->flags.filter_warnout; }
};

#pragma pack(pop)

class TionLtApi : public TionApiBase<tionlt_state_t> {
 public:
  void read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size);

  uint16_t get_state_type() const;

  bool request_dev_info() const;
  bool request_state() const;

  bool write_state(const tionlt_state_t &state, uint32_t request_id) const;
  bool reset_filter(const tionlt_state_t &state, uint32_t request_id = 1) const;
  bool factory_reset(const tionlt_state_t &state, uint32_t request_id = 1) const;

#ifdef TION_ENABLE_HEARTBEAT
  bool send_heartbeat() const { return false; }
#endif
};

}  // namespace tion
}  // namespace dentra
