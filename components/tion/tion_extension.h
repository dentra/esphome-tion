#pragma once

namespace esphome {
namespace tion {
template<class T> class EntityExtension : public T {
 public:
  void ext_set_internal(bool internal) { this->flags_.internal = internal; }
};
}  // namespace tion
}  // namespace esphome
