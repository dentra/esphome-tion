#pragma once

#include <cstddef>
#include <vector>
#include "esphome/core/component.h"

namespace esphome {
namespace vport {

template<typename T> class VPortListener {
 public:
  /// Read the data array from vport with specified type and size.
  virtual void on_frame_data(T type, const void *data, size_t size) = 0;
};

template<typename T> class VPort {
 public:
  void add_listener(VPortListener<T> *listener) { this->listeners_.push_back(listener); }

  void fire_listeners(T type, const void *data, size_t size) const {
    for (auto listener : this->listeners_) {
      listener->on_frame_data(type, data, size);
    }
  }

 protected:
  std::vector<VPortListener<T> *> listeners_;
};

template<typename T> class VPortComponent : public VPort<T>, public PollingComponent {};

}  // namespace vport
}  // namespace esphome
