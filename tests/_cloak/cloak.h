#pragma once

#include <vector>
#include <string>
#include <set>

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/component.h"

#define __FILENAME__ (__builtin_strrchr("/" __FILE__, '/') + 1)
#define DEFINE_TAG static const char *const TAG = __FILENAME__

namespace cloak {

typedef bool (*test_fn_t)();
void register_test(const std::string &name, test_fn_t fn);
bool run_tests(const std::set<std::string> &run_only);
int run_tests(int argc, char const *argv[]);

#define REGISTER_TEST(test) \
  struct test##_reg_t { \
    test##_reg_t() { cloak::register_test(#test, &test); } \
  } test##_reg;

std::vector<uint8_t> from_hex(const std::string &hex);

inline std::string hexencode(const void *data, size_t size) {
  return esphome::format_hex_pretty(static_cast<const uint8_t *>(data), size);
}

inline std::string hexencode(std::vector<uint8_t> &c) { return esphome::format_hex_pretty(c.data(), c.size()); }
inline std::string hexencode(std::set<uint8_t> &c) {
  std::vector<uint8_t> v(c.begin(), c.end());
  return esphome::format_hex_pretty(v.data(), v.size());
}

template<typename T> using enable_if_data_class_t = std::enable_if_t<std::is_class_v<T>, bool>;

template<class T, enable_if_data_class_t<T> = true> std::string hexencode(const T &data) {
  return esphome::format_hex_pretty(&data, sizeof(data));
}

template<class T, enable_if_data_class_t<T> = true> std::vector<uint8_t> from_data(const T &data) {
  auto data8 = reinterpret_cast<const uint8_t *>(&data);
  return std::vector(data8, data8 + sizeof(data));
}

class Cloak {
 public:
  std::string test_data_str() { return esphome::format_hex_pretty(this->cloak_data_.data(), this->cloak_data_.size()); }

  const std::vector<uint8_t> &test_data() const { return this->cloak_data_; }
  void test_data_clear() { this->cloak_data_.clear(); }
  virtual void test_data_push(const uint8_t *data, size_t size) {}
  virtual void test_data_push(const std::vector<uint8_t> &data) { this->test_data_push(data.data(), data.size()); }

 protected:
  std::vector<uint8_t> cloak_data_;
};

namespace internal {

int char2int(char ch);

bool test(const char *tag, const std::string &name, bool data1, bool data2);
bool test(const char *tag, const std::string &name, uint32_t data1, uint32_t data2);
bool test(const char *tag, const std::string &name, int32_t data1, int32_t data2);
bool test(const char *tag, const std::string &name, double data1, double data2);
bool test(const char *tag, const std::string &name, const std::string data1, const std::string data2);

bool test(const char *tag, const std::string &name, const std::vector<uint8_t> &data1,
          const std::vector<uint8_t> &data2);

inline bool test(const char *tag, const std::string &name, const std::vector<uint8_t> &data1,
                 const std::string &data2) {
  return test(tag, name, data1, from_hex(data2));
}
inline bool test(const char *tag, const std::string &name, cloak::Cloak *data1, const std::vector<uint8_t> &data2) {
  bool res = test(tag, name, data1->test_data(), data2);
  data1->test_data_clear();
  return res;
}
inline bool test(const char *tag, const std::string &name, cloak::Cloak *data1, const std::string &data2) {
  return test(tag, name, data1, from_hex(data2));
}
inline bool test(const char *tag, const std::string &name, cloak::Cloak &data1, const std::vector<uint8_t> &data2) {
  return test(tag, name, &data1, data2);
}
inline bool test(const char *tag, const std::string &name, cloak::Cloak &data1, const std::string &data2) {
  return test(tag, name, &data1, from_hex(data2));
}
}  // namespace internal

#define check_data(name, data1, data2) internal::test(TAG, name, data1, data2)
#define check_data_(data1, data2) check_data(#data1 " == " #data2, data1, data2)

inline void setup_and_loop(std::vector<esphome::Component *> components) {
  for (auto c : components) {
    c->call_setup();
  }
  for (auto c : components) {
    c->call_dump_config();
  }
  for (auto c : components) {
    c->call_loop();
  }
}

};  // namespace cloak

#define hexencode_cstr(...) cloak::hexencode(__VA_ARGS__).c_str()
