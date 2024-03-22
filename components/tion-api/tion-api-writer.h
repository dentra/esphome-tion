#pragma once

#pragma once

#include <cinttypes>
#include <etl/delegate.h>

namespace dentra {
namespace tion {

class TionApiWriter {
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

}  // namespace tion
}  // namespace dentra
