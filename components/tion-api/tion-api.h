#pragma once

#include <cstddef>
#include <vector>
#include <type_traits>

#include <etl/delegate.h>

namespace dentra {
namespace tion {

#pragma pack(push, 1)
// NOLINTNEXTLINE(readability-identifier-naming)
struct tion_dev_info_t {
  // NOLINTNEXTLINE(readability-identifier-naming)
  enum work_mode_t : uint8_t {
    UNKNOWN = 0,
    // обычный режим работы
    NORMAL = 1,
    // бризер находится в режиме обновления
    UPDATE = 2,
  } work_mode;
  // NOLINTNEXTLINE(readability-identifier-naming)
  enum device_type_t : uint32_t {
    BRO2 = 1,
    BR3S = 2,
    // Tion IQ 200
    IQ200 = 0x8001,
    // Tion Lite
    BRLT = 0x8002,
    // Tion 4S
    BR4S = 0x8003,
  } device_type;
  uint16_t firmware_version;
  uint16_t hardware_version;
  uint8_t reserved[16];
};

// NOLINTNEXTLINE(readability-identifier-naming)
template<tion_dev_info_t::device_type_t DT> struct tion_state_counters_t {
  // Motor time counter in seconds. power_up_time
  uint32_t work_time;
  // Electronics time count in seconds.
  uint32_t fan_time;
  // Filter time counter in seconds.
  uint32_t filter_time;
  // Airflow counter, m3=airflow_counter * 15.0 / 3600.0.
  uint32_t airflow_counter;
  // Calculated airflow in m3.
  float airflow() const { return airflow_counter * (float(airflow_k()) / 3600.0f); }
  // Calculated filter days left
  uint32_t filter_time_left() const { return filter_time / (24 * 3600); }

  constexpr size_t airflow_k() const {
    if constexpr (DT == tion_dev_info_t::device_type_t::BRLT) {
      return 10;
    }
    return 15;
  }
};

#pragma pack(pop)

class TionApiBaseWriter {
 public:
  using writer_type = etl::delegate<bool(uint16_t type, const void *data, size_t size)>;
  void set_writer(writer_type &&writer) { this->writer_ = writer; }

  // Write any frame data.
  bool write_frame(uint16_t type, const void *data, size_t size) const;
  /// Write a frame with empty data.
  bool write_frame(uint16_t type) const { return this->write_frame(type, nullptr, 0); }
  /// Write a frame with empty data and request_id (4S and Lite only).
  bool write_frame(uint16_t type, uint32_t request_id) const {
    return this->write_frame(type, &request_id, sizeof(request_id));
  }
  /// Write a frame data struct.
  template<class T, std::enable_if_t<std::is_class_v<T>, bool> = true>
  bool write_frame(uint16_t type, const T &data) const {
    return this->write_frame(type, &data, sizeof(data));
  }
  /// Write a frame data struct with request id (4S and Lite only).
  template<class T, std::enable_if_t<std::is_class_v<T>, bool> = true>
  bool write_frame(uint16_t type, const T &data, uint32_t request_id) const {
    struct {
      uint32_t request_id;
      T data;
    } __attribute__((__packed__)) req{.request_id = request_id, .data = data};
    return this->write_frame(type, &req, sizeof(req));
  }

 protected:
  writer_type writer_{};
};

template<class state_type_t> class TionApiBase : public TionApiBaseWriter {
  /// Callback listener for response to request_dev_info command request.
  using on_dev_info_type = etl::delegate<void(const tion_dev_info_t &dev_info)>;
  /// Callback listener for response to request_state command request.
  using on_state_type = etl::delegate<void(const state_type_t &state, uint32_t request_id)>;
  /// Callback listener for response to send_heartbeat command request.
  using on_heartbeat_type = etl::delegate<void(tion_dev_info_t::work_mode_t work_mode)>;

 public:
  using state_type = state_type_t;

  using on_ready_type = etl::delegate<void()>;
  /// Set callback listener for monitoring ready state
  void set_on_ready(on_ready_type &&on_ready) { this->on_ready_ = on_ready; }

  on_dev_info_type on_dev_info{};
  on_state_type on_state{};
#ifdef TION_ENABLE_HEARTBEAT
  on_heartbeat_type on_heartbeat{};
#endif
 protected:
  on_ready_type on_ready_{};
};

// NOLINTNEXTLINE(readability-identifier-naming)
template<class data_type> struct tion_frame_t {
  uint16_t type;
  data_type data;
  constexpr static size_t head_size() { return sizeof(type); }
} __attribute__((__packed__));
using tion_any_frame_t = tion_frame_t<uint8_t[0]>;

// NOLINTNEXTLINE(readability-identifier-naming)
template<class data_type> struct tion_ble_frame_t {
  uint16_t type;
  uint32_t ble_request_id;  // always 1
  data_type data;
  constexpr static size_t head_size() { return sizeof(type) + sizeof(ble_request_id); }
} __attribute__((__packed__));

using tion_any_ble_frame_t = tion_ble_frame_t<uint8_t[0]>;

template<class frame_spec_t> class TionProtocol {
 public:
  using frame_spec_type = frame_spec_t;

  using reader_type = etl::delegate<void(const frame_spec_t &data, size_t size)>;
  // TODO move to protected
  reader_type reader{};
  void set_reader(reader_type &&reader) { this->reader = reader; }

  using writer_type = etl::delegate<bool(const uint8_t *data, size_t size)>;
  // TODO move to protected
  writer_type writer{};
  void set_writer(writer_type &&writer) { this->writer = writer; }
};

}  // namespace tion
}  // namespace dentra
