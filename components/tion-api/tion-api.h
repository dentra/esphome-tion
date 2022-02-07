#pragma once

#include <inttypes.h>
#include <vector>

namespace dentra {
namespace tion {

#pragma pack(push, 1)
struct tion_state_counters_t {
  // Motor time counter in seconds. power_up_time
  uint32_t work_time;
  // Electronics time count in seconds.
  uint32_t fan_time;
  // Filter time counter in seconds.
  uint32_t filter_time;
  // Airflow counter, m3=airflow_counter * 15.0 / 3600.0.
  uint32_t airflow_counter_;
  // Calculated airflow_counter in m3.
  float airflow_counter() const { return airflow_counter_ * (15.0f / 3600.0f); }
  // Calculated filter days left
  uint32_t fileter_days() const { return filter_time / 86400; }
};

struct tion_dev_status_t {
  uint8_t work_mode;  // system_mode
  enum : uint16_t { IRM = 0x8001, BRLT = 0x8002, BR4S = 0x8003 } device_type;
  uint16_t device_subtype;  // always 0
  uint16_t firmware_version;
  uint16_t hardware_version;
  uint8_t reserved[16];
};

#pragma pack(pop)

class TionApi {
 public:
  // main point to read data from BLE
  void read_data(const uint8_t *data, uint16_t size);
  // main point to read data from BLE
  void read_data(const std::vector<uint8_t> &data) { this->read_data(data.data(), data.size()); }
  // main point to write data to BLE
  virtual bool write_data(const uint8_t *data, uint16_t size) const = 0;

  // virtual void read(const tion_dev_status_t &status) = 0;

  virtual void request_dev_status() = 0;
  virtual void request_state() = 0;

 protected:
  std::vector<uint8_t> rx_buf_;

  // the loweest level of reading data. read BLE packets
  void read_packet_(uint8_t packet_type, const uint8_t *data, uint16_t size);
  // the middle level of reading data
  void read_frame_(const void *data, uint32_t size);
  // the highest level of reading data
  virtual void read_(uint16_t frame_type, const void *frame_data, uint16_t frame_data_size) = 0;

  // write any frame data
  bool write_frame_(uint16_t frame_type, const void *frame_data, uint16_t frame_data_size) const;

  // write a frame with empty data
  bool write_(uint16_t frame_type) const { return this->write_frame_(frame_type, nullptr, 0); }

  // write a frame data
  template<class T> bool write_(uint16_t frame_type, const T &frame_data) const {
    return this->write_frame_(frame_type, &frame_data, sizeof(frame_data));
  }

  // write a frame data with command id
  template<class T> bool write_(uint16_t frame_type, const T &frame_data, uint32_t command_id) const {
    struct {
      uint32_t command_id;
      T data;
    } __attribute__((packed)) req{.command_id = command_id, .data = frame_data};
    return this->write_frame_(frame_type, &req, sizeof(req));
  }

  bool write_packet_(const void *data, uint16_t size) const;

  uint32_t next_command_id() const { return 1; }
};

}  // namespace tion
}  // namespace dentra
