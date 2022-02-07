#pragma once

#include "tion-api.h"

namespace dentra {
namespace tion {

#pragma pack(push, 1)

struct _button_presets {
  enum { PRESET_NUMBER = 3 };
  int8_t temp[PRESET_NUMBER];
  uint8_t fan_speed[PRESET_NUMBER];
};

struct tionlt_state_t {
  struct {
    struct {
      // состояние (power state)
      bool power_state : 1;
      // состояние звуковых оповещений
      bool sound_state : 1;
      // состояние световых оповещений
      bool led_state : 1;
      //
      uint8_t last_com_source : 1;
      // предупреждение о необходимости замены фильтра
      bool filter_wornout : 1;
      //
      uint8_t auto_co2 : 1;
      //
      uint8_t heater_state : 1;
      //
      uint8_t heater_present : 1;
      // the presence of the KIV mode
      uint8_t kiv_present : 1;
      // state of the KIV mode
      uint8_t kiv_active : 1;
      // reserved
      uint8_t reserved : 6;
    };
    uint8_t gate_position;  // gate_state
    int8_t target_temperature;
    uint8_t fan_speed;
  } system;
  struct {
    int8_t indoor_temperature;
    int8_t outdoor_temperature;
    int8_t pcb_temperature;
  } sensors;
  tion_state_counters_t counters;
  struct {
    uint32_t reg;
    struct {
      enum { ERROR_TYPE_NUMBER = 20 };
      uint8_t er[ERROR_TYPE_NUMBER];
    } cnt;
  } errors;
  _button_presets button_presets;
  struct {
    uint8_t max_fan_speed;
  } limits;
  uint8_t heater_var;
  uint8_t test_type;
  float heater_power() const;
};

#pragma pack(pop)

class TionApiLt : public TionApi {
 public:
  virtual void read(const tion_dev_status_t &status) {}
  virtual void read(const tionlt_state_t &state) {}

  void request_dev_status() override;
  void request_state() override;

  bool write_state(const tionlt_state_t &state) const;
  bool reset_filter(const tionlt_state_t &state) const;
  bool factory_reset(const tionlt_state_t &state) const;

 protected:
  void read_(uint16_t frame_type, const void *frame_data, uint16_t frame_data_size) override;
};

}  // namespace tion
}  // namespace dentra
