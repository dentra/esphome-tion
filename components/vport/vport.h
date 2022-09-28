#pragma once

#include <cstddef>
#include <vector>
#include <type_traits>
#include <etl/delegate.h>

namespace esphome {
namespace vport {

template<typename frame_type> class VPortCodec {
 public:
  bool decode(const uint8_t *data, size_t size);
  bool encode(frame_type type, const void *data, size_t size);
};

template<typename frame_type> class VPort {
  using on_ready_type = etl::delegate<bool()>;
  using on_update_type = etl::delegate<bool()>;
  using on_frame_type = etl::delegate<bool(frame_type type, const void *data, size_t size)>;

 public:
  void fire_frame(frame_type type, const void *data, size_t size) const {
    if (this->on_frame) {
      this->on_frame(type, data, size);
    }
  }

  void fire_poll() {
    if (this->on_update) {
      this->on_update();
    }
  }

  void fire_ready() {
    if (this->on_ready) {
      if (!this->on_ready()) {
        return;
      }
    }
    if (this->on_update) {
      this->on_update();
    }
  }

  on_ready_type on_ready{};
  on_update_type on_update{};
  on_frame_type on_frame{};
};

}  // namespace vport
}  // namespace esphome
