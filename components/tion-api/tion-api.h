#pragma once

#include <cstddef>
#include <vector>
#include <type_traits>

#include <etl/delegate.h>

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
  enum : uint32_t { IRM = 0x8001, BRLT = 0x8002, BR4S = 0x8003 } device_type;
  uint16_t firmware_version;
  uint16_t hardware_version;
  uint8_t reserved[16];
};

#pragma pack(pop)

class TionApiBaseWriter {
  using writer_type = etl::delegate<bool(uint16_t type, const void *data, size_t size)>;

 public:
  writer_type writer{};

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
};

template<class state_type> class TionApiBase : public TionApiBaseWriter {
  /// Callback listener for response to request_dev_status command request.
  using on_dev_status_type = etl::delegate<void(const tion_dev_status_t &dev_status)>;
  /// Callback listener for response to request_state command request.
  using on_state_type = etl::delegate<void(const state_type &state, uint32_t request_id)>;
  /// Callback listener for response to send_heartbeat command request.
  using on_heartbeat_type = etl::delegate<void(uint8_t unknown)>;

 public:
  on_dev_status_type on_dev_status{};
  on_state_type on_state{};
#ifdef TION_ENABLE_HEARTBEAT
  on_heartbeat_type on_heartbeat{};
#endif
};

class TionProtocol {
  using writer_type = etl::delegate<bool(const uint8_t *data, size_t size)>;
  using reader_type = etl::delegate<bool(uint16_t type, const void *data, size_t size)>;

 public:
  reader_type reader{};
  writer_type writer{};
};

}  // namespace tion
}  // namespace dentra
