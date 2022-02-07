
#include <cstdlib>
#include <ctime>
#include <cstring>

#include "utils.h"

static int char2int(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  }
  if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  }
  return -1;
}

// converts hex string to bytes skip spaces, dots and dashes if exist.
std::vector<uint8_t> from_hex(const std::string &hex) {
  std::vector<uint8_t> res;
  for (const char *ptr = hex.c_str(), *end = ptr + hex.length(); ptr < end; ptr++) {
    if (*ptr == ' ' || *ptr == '.' || *ptr == '-' || *ptr == ':') {
      continue;
    }
    auto byte = char2int(*ptr) << 4;
    ptr++;
    byte += char2int(*ptr);
    res.push_back(byte);
  }
  return res;
}

std::string hex(const void *data, uint32_t size, char sep, bool add_size) {
  static const char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  const uint8_t *data_ptr = static_cast<const uint8_t *>(data);
  std::string str;
  if (size > 0) {
    int mul = 2;
    if (sep != 0) {
      str.resize(size * (++mul) - 1);
    } else {
      str.resize(size * mul);
    }
    for (int i = 0; i < size; ++i) {
      str[mul * i + 0] = hexmap[(*data_ptr & 0xF0) >> 4];
      str[mul * i + 1] = hexmap[(*data_ptr & 0x0F) >> 0];
      if (sep != 0 && i < size - 1) {
        str[mul * i + 2] = sep;
      }
      data_ptr++;
    }
    str += ' ';
  }

  if (add_size) {
    str += '(';
    str += std::to_string(size);
    str += ')';
  }
  return str;
}

uint8_t fast_random_8() {
  // std::random_device r;
  // std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
  // std::mt19937 eng{seed};
  // std::uniform_int_distribution<> dist(0, 255);
  // return dist(eng);

  static bool init = false;
  if (!init) {
    std::srand(std::time(0));
    init = !init;
  }

  constexpr auto minValue = 0;
  constexpr auto maxValue = 255;
  return std::rand() % (maxValue - minValue + 1) + minValue;
}

#define ESPHOME_LOG_COLOR_BLACK "\e[1;30m"
#define ESPHOME_LOG_COLOR_RED "\e[1;31m"     // ERROR
#define ESPHOME_LOG_COLOR_GREEN "\e[1;32m"   // INFO
#define ESPHOME_LOG_COLOR_YELLOW "\e[1;33m"  // WARNING
#define ESPHOME_LOG_COLOR_BLUE "\e[1;34m"
#define ESPHOME_LOG_COLOR_MAGENTA "\e[1;35m"  // CONFIG
#define ESPHOME_LOG_COLOR_CYAN "\e[1;36m"     // DEBUG
#define ESPHOME_LOG_COLOR_GRAY "\e[1;37m"     // VERBOSE
#define ESPHOME_LOG_COLOR_WHITE "\e[1;38m"
#define ESPHOME_LOG_SECRET_BEGIN "\033[5m"
#define ESPHOME_LOG_SECRET_END "\033[6m"

void printf_logger(int level, const char *tag, int line, const char *format, va_list va) {
  static const char *const levels[] = {
      "NONE   ",
      ESPHOME_LOG_COLOR_RED "ERROR  ",      // 1
      ESPHOME_LOG_COLOR_YELLOW "WARN   ",   // 2
      ESPHOME_LOG_COLOR_GREEN "INFO   ",    // 3
      ESPHOME_LOG_COLOR_MAGENTA "CONFIG ",  // 4
      ESPHOME_LOG_COLOR_CYAN "DEBUG  ",     // 5
      ESPHOME_LOG_COLOR_GRAY "VERBOSE",     // 6
  };
  printf("%s [%s:%u] ", levels[level], tag, line);
  vprintf(format, va);
  printf("\e[0m\n");
}

void __attribute__((hot)) log_printf_(int level, const char *tag, int line, const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  printf_logger(level, tag, line, format, arg);
  va_end(arg);
}

void _test_check_failed(const char *file, int line, const char *msg, bool actual, bool expected) {
  log_printf_(1, file, line, "FAILED  %s, actual = 0x%02X (%u), expected = 0x%02X (%u)", msg, actual, actual, expected,
              expected);
}
void _test_check_success(const char *file, int line, const char *msg, bool actual, bool expected) {
  log_printf_(3, file, line, "SUCCESS %s, value = 0x%02X (%u)", msg, expected, expected);
}

void _test_check_failed(const char *file, int line, const char *msg, uint8_t actual, uint8_t expected) {
  log_printf_(1, file, line, "FAILED  %s, actual = 0x%02X (%u), expected = 0x%02X (%u)", msg, actual, actual, expected,
              expected);
}
void _test_check_success(const char *file, int line, const char *msg, uint8_t actual, uint8_t expected) {
  log_printf_(3, file, line, "SUCCESS %s, value = 0x%02X (%u)", msg, expected, expected);
}

void _test_check_failed(const char *file, int line, const char *msg, uint16_t actual, uint16_t expected) {
  log_printf_(1, file, line, "FAILED  %s, actual = 0x%04X (%u), expected = 0x%04X (%u)", msg, actual, actual, expected,
              expected);
}
void _test_check_success(const char *file, int line, const char *msg, uint16_t actual, uint16_t expected) {
  log_printf_(3, file, line, "SUCCESS %s, value = 0x%04X (%u)", msg, expected, expected);
}

void _test_check_failed(const char *file, int line, const char *msg, const std::vector<uint8_t> &actual,
                        const std::vector<uint8_t> &expected) {
  log_printf_(1, file, line,
              "FAILED  %s,\n"
              "  actual   = %s,\n"
              "  expected = %s",
              msg, hexencode(actual.data(), actual.size()).c_str(),
              hexencode(expected.data(), expected.size()).c_str());
}

void _test_check_success(const char *file, int line, const char *msg, const std::vector<uint8_t> &actual,
                         const std::vector<uint8_t> &expected) {
  log_printf_(3, file, line,
              "SUCCESS  %s,\n"
              "  actual   = %s,\n"
              "  expected = %s",
              msg, hexencode(actual.data(), actual.size()).c_str(),
              hexencode(expected.data(), expected.size()).c_str());
}
