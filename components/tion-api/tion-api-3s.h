#pragma once

#include <vector>

namespace dentra {
namespace tion {

#pragma pack(push, 1)
struct tion3s_state_t {
  enum GatePosition : uint8_t { GATE_POSITION_INDOOR = 0, GATE_POSITION_MIXED = 1, GATE_POSITION_OUTDOOR = 2 };
  uint8_t fan_speed : 4;
  uint8_t /*GatePosition*/ gate_position : 4;
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
  int8_t outdoor_temperature1;
  int8_t outdoor_temperature2;
  int8_t indoor_temperature;
  uint16_t filter_time;
  uint8_t hours;
  uint8_t minutes;
  uint8_t last_error;
  uint8_t productivity;  // m3 per hour;
  uint16_t filter_days;
  uint16_t firmware_version;
};
#pragma pack(pop)

class TionsApi3s {
 public:
  virtual void read(const tion3s_state_t &state) {}

  bool pair() const;
  bool request_state() const;
  bool write_state(const tion3s_state_t &state) const;
  bool reset_filter(const tion3s_state_t &state) const;

  // main point to read data from BLE
  bool read_data(const uint8_t *data, uint16_t size);
  // main point to read data from BLE
  bool read_data(const std::vector<uint8_t> &data) { return this->read_data(data.data(), data.size()); }

  // main point to write data to BLE
  virtual bool write_data(const uint8_t *data, uint16_t size) const = 0;

 protected:
  // write any frame data
  bool write_frame_(uint16_t frame_type, const void *frame_data, uint16_t frame_data_size) const;

  // write a frame with empty data
  bool write_(uint16_t frame_type) const { return this->write_frame_(frame_type, nullptr, 0); }
};

}  // namespace tion
}  // namespace dentra
