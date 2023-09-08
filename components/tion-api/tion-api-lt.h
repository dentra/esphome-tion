#pragma once

#include "tion-api.h"

namespace dentra {
namespace tion {

#pragma pack(push, 1)

struct tionlt_state_t {
  struct {
    // состояние (power state)
    bool power_state : 1;
    // состояние звуковых оповещений
    bool sound_state : 1;
    // состояние световых оповещений
    bool led_state : 1;
    uint8_t last_com_source : 1;
    // предупреждение о необходимости замены фильтра
    bool filter_warnout : 1;
    bool auto_co2 : 1;
    bool heater_state : 1;
    bool heater_present : 1;
    // the presence of the KIV mode
    bool kiv_present : 1;
    // state of the KIV mode
    bool kiv_active : 1;
    // reserved
    uint8_t reserved : 6;
  } flags;
  uint8_t gate_position;  // gate_state
  int8_t target_temperature;
  uint8_t fan_speed;
  int8_t outdoor_temperature;
  int8_t current_temperature;
  int8_t pcb_temperature;
  tion_state_counters_t counters;
  struct {
    uint32_t reg;
    struct {
      enum { ERROR_TYPE_NUMBER = 20 };
      uint8_t er[ERROR_TYPE_NUMBER];
    } cnt;
  } errors;
  struct button_presets_t {
    enum { PRESET_NUMBER = 3 };
    int8_t temp[PRESET_NUMBER];
    uint8_t fan_speed[PRESET_NUMBER];
  } button_presets;
  uint8_t max_fan_speed;
  uint8_t heater_var;
  uint8_t test_type;
  float heater_power() const;
  bool is_initialized() const { return this->counters.work_time != 0; }
};

#pragma pack(pop)

class TionApiLt : public TionApiBase<tionlt_state_t> {
 public:
  void read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size);

  uint16_t get_state_type() const;

  bool request_dev_info() const;
  bool request_state() const;

  bool write_state(const tionlt_state_t &state, uint32_t request_id = 1) const;
  bool reset_filter(const tionlt_state_t &state, uint32_t request_id = 1) const;
  bool factory_reset(const tionlt_state_t &state, uint32_t request_id = 1) const;

#ifdef TION_ENABLE_HEARTBEAT
  bool send_heartbeat() const { return false; }
#endif
};

}  // namespace tion
}  // namespace dentra
