#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <stdio.h>
#include <stdarg.h>  // va_list

#define ONOFF(b) ((b) ? "ON" : "OFF")
#define TRUEFALSE(b) ((b) ? "TRUE" : "FALSE")

#define PACKED __attribute__((packed))

using test_fn_t = std::function<bool(bool)>;

int char2int(char ch);
std::vector<uint8_t> from_hex(const std::string &hex);
std::string hex(const void *data, uint32_t size, char sep = 0, bool add_size = false);
inline std::string hexencode(const void *data, uint32_t size) { return hex(data, size, '.', true); }

void printf_logger(int level, const char *tag, int line, const char *format, va_list va);

void log_printf_(int level, const char *tag, int line, const char *format, ...) __attribute__((format(printf, 4, 5)));

void _test_check_failed(const char *file, int line, const char *msg, bool actual, bool expected);
void _test_check_success(const char *file, int line, const char *msg, bool actual, bool expected);
void _test_check_failed(const char *file, int line, const char *msg, uint8_t actual, uint8_t expected);
void _test_check_success(const char *file, int line, const char *msg, uint8_t actual, uint8_t expected);
void _test_check_failed(const char *file, int line, const char *msg, uint16_t actual, uint16_t expected);
void _test_check_success(const char *file, int line, const char *msg, uint16_t actual, uint16_t expected);
void _test_check_failed(const char *file, int line, const char *msg, const std::vector<uint8_t> &actual,
                        const std::vector<uint8_t> &expected);
void _test_check_success(const char *file, int line, const char *msg, const std::vector<uint8_t> &actual,
                         const std::vector<uint8_t> &expected);

#define test_check(res, actual, expected) \
  { \
    auto __actual_res = (actual); \
    auto __expected_res = (expected); \
    if ((__actual_res) != (__expected_res)) { \
      _test_check_failed(__FILE__, __LINE__, #actual " != " #expected, __actual_res, __expected_res); \
      res = false; \
    } else { \
      _test_check_success(__FILE__, __LINE__, #actual " == " #expected, __actual_res, __expected_res); \
    } \
  }

#define LOGD(fmt, ...) log_printf_(5, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) log_printf_(3, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) log_printf_(2, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) log_printf_(1, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define ESP_LOGV(TAG, fmt, ...) \
  log_printf_(6, __FILE__, __LINE__, (std::string(TAG) + ": " + fmt).c_str(), ##__VA_ARGS__)
#define ESP_LOGD(TAG, fmt, ...) \
  log_printf_(5, __FILE__, __LINE__, (std::string(TAG) + ": " + fmt).c_str(), ##__VA_ARGS__)
#define ESP_LOGI(TAG, fmt, ...) \
  log_printf_(3, __FILE__, __LINE__, (std::string(TAG) + ": " + fmt).c_str(), ##__VA_ARGS__)
#define ESP_LOGW(TAG, fmt, ...) \
  log_printf_(2, __FILE__, __LINE__, (std::string(TAG) + ": " + fmt).c_str(), ##__VA_ARGS__)
#define ESP_LOGE(TAG, fmt, ...) \
  log_printf_(1, __FILE__, __LINE__, (std::string(TAG) + ": " + fmt).c_str(), ##__VA_ARGS__)

uint8_t fast_random_8();

std::string str_snprintf(const char *fmt, size_t len, ...);

// minimal C++11 allocator with debug output
template<class Tp> struct NAlloc {
  typedef Tp value_type;
  NAlloc() = default;
  template<class T> NAlloc(const NAlloc<T> &) {}

  Tp *allocate(std::size_t n) {
    n *= sizeof(Tp);
    Tp *p = static_cast<Tp *>(::operator new(n));
    std::cout << "allocating " << n << " bytes @ " << p << std::endl;
    return p;
  }

  void deallocate(Tp *p, std::size_t n) {
    std::cout << "deallocating " << n * sizeof *p << " bytes @ " << p << std::endl;
    ::operator delete(p);
  }
};
template<class T, class U> bool operator==(const NAlloc<T> &, const NAlloc<U> &) { return true; }
template<class T, class U> bool operator!=(const NAlloc<T> &, const NAlloc<U> &) { return false; }

namespace esphome {
namespace time {

/// A more user-friendly version of struct tm from time.h
struct ESPTime {
  /** seconds after the minute [0-60]
   * @note second is generally 0-59; the extra range is to accommodate leap seconds.
   */
  uint8_t second;
  /// minutes after the hour [0-59]
  uint8_t minute;
  /// hours since midnight [0-23]
  uint8_t hour;
  /// day of the week; sunday=1 [1-7]
  uint8_t day_of_week;
  /// day of the month [1-31]
  uint8_t day_of_month;
  /// day of the year [1-366]
  uint16_t day_of_year;
  /// month; january=1 [1-12]
  uint8_t month;
  /// year
  uint16_t year;
  /// daylight saving time flag
  bool is_dst;
  /// unix epoch time (seconds since UTC Midnight January 1, 1970)
  time_t timestamp;

  /** Convert this ESPTime struct to a null-terminated c string buffer as specified by the format argument.
   * Up to buffer_len bytes are written.
   *
   * @see https://www.gnu.org/software/libc/manual/html_node/Formatting-Calendar-Time.html#index-strftime
   */
  size_t strftime(char *buffer, size_t buffer_len, const char *format);

  /** Convert this ESPTime struct to a string as specified by the format argument.
   * @see https://www.gnu.org/software/libc/manual/html_node/Formatting-Calendar-Time.html#index-strftime
   *
   * @warning This method uses dynamically allocated strings which can cause heap fragmentation with some
   * microcontrollers.
   */
  std::string strftime(const std::string &format);

  /// Check if this ESPTime is valid (all fields in range and year is greater than 2018)
  bool is_valid() const { return this->year >= 2019 && this->fields_in_range(); }

  /// Check if all time fields of this ESPTime are in range.
  bool fields_in_range() const {
    return this->second < 61 && this->minute < 60 && this->hour < 24 && this->day_of_week > 0 &&
           this->day_of_week < 8 && this->day_of_month > 0 && this->day_of_month < 32 && this->day_of_year > 0 &&
           this->day_of_year < 367 && this->month > 0 && this->month < 13;
  }

  /// Convert a C tm struct instance with a C unix epoch timestamp to an ESPTime instance.
  static ESPTime from_c_tm(struct tm *c_tm, time_t c_time);

  /** Convert an UTC epoch timestamp to a local time ESPTime instance.
   *
   * @param epoch Seconds since 1st January 1970. In UTC.
   * @return The generated ESPTime
   */
  static ESPTime from_epoch_local(time_t epoch) {
    struct tm *c_tm = ::localtime(&epoch);
    return ESPTime::from_c_tm(c_tm, epoch);
  }
  /** Convert an UTC epoch timestamp to a UTC time ESPTime instance.
   *
   * @param epoch Seconds since 1st January 1970. In UTC.
   * @return The generated ESPTime
   */
  static ESPTime from_epoch_utc(time_t epoch) {
    struct tm *c_tm = ::gmtime(&epoch);
    return ESPTime::from_c_tm(c_tm, epoch);
  }

  /// Recalculate the timestamp field from the other fields of this ESPTime instance (must be UTC).
  void recalc_timestamp_utc(bool use_day_of_year = true);

  /// Convert this ESPTime instance back to a tm struct.
  struct tm to_c_tm();

  /// Increment this clock instance by one second.
  void increment_second();
  bool operator<(ESPTime other);
  bool operator<=(ESPTime other);
  bool operator==(ESPTime other);
  bool operator>=(ESPTime other);
  bool operator>(ESPTime other);
};

}  // namespace time
}  // namespace esphome
