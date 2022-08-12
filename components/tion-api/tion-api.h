#pragma once

#include <cstddef>
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
  uint32_t filter_days() const { return filter_time / 86400; }
};

struct tion_dev_status_t {
  uint8_t work_mode;  // system_mode
  enum : uint16_t { IRM = 0x8001, BRLT = 0x8002, BR4S = 0x8003 } device_type;
  uint16_t device_subtype;  // always 0
  uint16_t firmware_version;
  uint16_t hardware_version;
  uint8_t reserved[16];
};

struct tion_heartbeat_t {
  uint8_t unknown;  // always 1
};

#pragma pack(pop)

class TionFrameReader {
 public:
  virtual bool read_frame(uint16_t type, const void *data, size_t size) = 0;
};

class TionFrameWriter {
 public:
  virtual bool write_frame(uint16_t type, const void *data, size_t size) const = 0;
};

class TionApiBase : public TionFrameReader {
 public:
  explicit TionApiBase(TionFrameWriter *writer) : writer_(writer) {}

  virtual uint16_t get_state_type() const = 0;

  /// Write any frame data.
  bool write_frame(uint16_t type, const void *data, size_t size) const;
  /// Write a frame with empty data.
  bool write_frame(uint16_t type) const { return this->write_frame(type, nullptr, 0); }
  /// Write a frame with empty data and request_id (4S and Lite only).
  bool write_frame(uint16_t type, uint32_t request_id) const {
    return this->write_frame(type, &request_id, sizeof(request_id));
  }
  /// Write a frame data struct.
  template<class T, typename std::enable_if<std::is_class<T>::value, bool>::type = true>
  bool write_frame(uint16_t type, const T &data) const {
    return this->write_frame(type, &data, sizeof(data));
  }
  /// Write a frame data struct with request id (4S and Lite only).
  template<class T, typename std::enable_if<std::is_class<T>::value, bool>::type = true>
  bool write_frame(uint16_t type, const T &data, uint32_t request_id) const {
    struct {
      uint32_t request_id;
      T data;
    } __attribute__((__packed__)) req{.request_id = request_id, .data = data};
    return this->write_frame(type, &req, sizeof(req));
  }

 protected:
  TionFrameWriter *writer_;
};

template<class S> class TionApi : public TionApiBase {
 public:
  explicit TionApi(TionFrameWriter *writer) : TionApiBase(writer) {}

  /// Response to send_heartbeat command request.
  virtual void on_heartbeat(const tion_heartbeat_t &heartbeat) {}
  /// Send heartbeat commmand request.
  virtual bool send_heartbeat() const { return false; }
  /// Response to request_dev_status command request.
  virtual void on_dev_status(const tion_dev_status_t &status) {}
  /// Request device for its status.
  virtual bool request_dev_status() const = 0;
  /// Response to request_state command request.
  virtual void on_state(const S &state, uint32_t request_id) {}
  /// Request device for its state.
  virtual bool request_state() const = 0;
};

class TionProtocol : public TionFrameWriter, public TionFrameReader {
 public:
  virtual bool write_data(const uint8_t *data, size_t size) const = 0;
};

}  // namespace tion
}  // namespace dentra
