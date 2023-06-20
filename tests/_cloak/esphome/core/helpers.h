#pragma once

#include <cmath>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "optional.h"

namespace esphome {

#define HOT __attribute__((hot))
#define ESPDEPRECATED(msg, when) __attribute__((deprecated(msg)))
#define ALWAYS_INLINE __attribute__((always_inline))
#define PACKED __attribute__((packed))

std::string hex(const void *data, uint32_t size, char sep, bool add_size);

inline std::string format_hex(const void *data, uint32_t size) { return hex(data, size, 0, false); }
inline std::string format_hex_pretty(const void *data, uint32_t size) { return hex(data, size, '.', size > 4); }
inline std::string format_hex_pretty(const std::vector<uint8_t> &data) {
  return format_hex_pretty(data.data(), data.size());
}

template<typename... X> class CallbackManager;

/** Helper class to allow having multiple subscribers to a callback.
 *
 * @tparam Ts The arguments for the callbacks, wrapped in void().
 */
template<typename... Ts> class CallbackManager<void(Ts...)> {
 public:
  /// Add a callback to the list.
  void add(std::function<void(Ts...)> &&callback) { this->callbacks_.push_back(std::move(callback)); }

  /// Call all callbacks in this manager.
  void call(Ts... args) {
    for (auto &cb : this->callbacks_)
      cb(args...);
  }

  /// Call all callbacks in this manager.
  void operator()(Ts... args) { call(args...); }

 protected:
  std::vector<std::function<void(Ts...)>> callbacks_;
};

using std::enable_if_t;
using std::is_trivially_copyable;
using std::is_invocable;

inline uint32_t millis() { return 0; }

inline void get_mac_address_raw(uint8_t *mac) { mac[0] = mac[1] = mac[2] = mac[3] = mac[4] = mac[5] = 0; }

std::string str_sprintf(const char *fmt, ...);
std::string str_snprintf(const char *fmt, size_t len, ...);

/// Encode a 16-bit value given the most and least significant byte.
constexpr uint16_t encode_uint16(uint8_t msb, uint8_t lsb) {
  return (static_cast<uint16_t>(msb) << 8) | (static_cast<uint16_t>(lsb));
}
/// Encode a 32-bit value given four bytes in most to least significant byte order.
constexpr uint32_t encode_uint32(uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4) {
  return (static_cast<uint32_t>(byte1) << 24) | (static_cast<uint32_t>(byte2) << 16) |
         (static_cast<uint32_t>(byte3) << 8) | (static_cast<uint32_t>(byte4));
}
/// Encode a 24-bit value given three bytes in most to least significant byte order.
constexpr uint32_t encode_uint24(uint8_t byte1, uint8_t byte2, uint8_t byte3) {
  return ((static_cast<uint32_t>(byte1) << 16) | (static_cast<uint32_t>(byte2) << 8) | (static_cast<uint32_t>(byte3)));
}

inline void delay(size_t ms) {}

/// Helper class to deduplicate items in a series of values.
template<typename T> class Deduplicator {
 public:
  /// Feeds the next item in the series to the deduplicator and returns whether this is a duplicate.
  bool next(T value) {
    if (this->has_value_) {
      if (this->last_value_ == value)
        return false;
    }
    this->has_value_ = true;
    this->last_value_ = value;
    return true;
  }
  /// Returns whether this deduplicator has processed any items so far.
  bool has_value() const { return this->has_value_; }

 protected:
  bool has_value_{false};
  T last_value_{};
};

uint32_t fnv1_hash(const std::string &str);
bool str_equals_case_insensitive(const std::string &a, const std::string &b);
int8_t step_to_accuracy_decimals(float step);

using std::to_string;

/// Helper class to easily give an object a parent of type \p T.
template<typename T> class Parented {
 public:
  Parented() {}
  Parented(T *parent) : parent_(parent) {}

  /// Get the parent of this object.
  T *get_parent() const { return parent_; }
  /// Set the parent of this object.
  void set_parent(T *parent) { parent_ = parent; }

 protected:
  T *parent_{nullptr};
};

}  // namespace esphome
