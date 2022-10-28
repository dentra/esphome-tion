#pragma once

#include <cstddef>
#include <vector>
#include <type_traits>
#include <etl/delegate.h>

namespace esphome {
namespace vport {

// TODO implement
// template<typename frame_type> class VPortCodec {
//  public:
//   bool decode(const uint8_t *data, size_t size);
//   bool encode(frame_type type, const void *data, size_t size);
// };

// TODO implement
// template<typename frame_type> class VPortProtocol {
//   using writer_type = etl::delegate<bool(const uint8_t *data, size_t size)>;
//   using reader_type = etl::delegate<bool(frame_type type, const void *data, size_t size)>;

//  public:
//   reader_type reader{};
//   writer_type writer{};
// };

template<typename frame_type> class VPort {
  using on_ready_type = etl::delegate<bool()>;
  using on_update_type = etl::delegate<bool()>;
  using on_frame_type = etl::delegate<bool(frame_type type, const void *data, size_t size)>;

 public:
  void fire_frame(frame_type type, const void *data, size_t size) const { this->on_frame(type, data, size); }

  void fire_poll() { this->on_update(); }

  void fire_ready() {
    if (this->on_ready()) {
      this->on_update();
    }
  }

  on_ready_type on_ready{[]() { return true; }};
  on_update_type on_update{[]() { return true; }};
  on_frame_type on_frame{[](frame_type, const void *, size_t) { return true; }};
};

}  // namespace vport
}  // namespace esphome
