#include <cstdarg>

#include "helpers.h"

namespace esphome {

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
  }

  if (add_size) {
    str += ' ';
    str += '(';
    str += std::to_string(size);
    str += ')';
  }
  return str;
}

std::string str_sprintf(const char *fmt, ...) {
  std::string str;
  va_list args;

  va_start(args, fmt);
  size_t length = vsnprintf(nullptr, 0, fmt, args);
  va_end(args);

  str.resize(length);
  va_start(args, fmt);
  vsnprintf(&str[0], length + 1, fmt, args);
  va_end(args);

  return str;
}

std::string str_snprintf(const char *fmt, size_t len, ...) {
  std::string str;
  va_list args;

  str.resize(len);
  va_start(args, len);
  size_t out_length = vsnprintf(&str[0], len + 1, fmt, args);
  va_end(args);

  if (out_length < len)
    str.resize(out_length);

  return str;
}

uint32_t fnv1_hash(const std::string &str) {
  uint32_t hash = 2166136261UL;
  for (char c : str) {
    hash *= 16777619UL;
    hash ^= c;
  }
  return hash;
}

bool str_equals_case_insensitive(const std::string &a, const std::string &b) {
  return strcasecmp(a.c_str(), b.c_str()) == 0;
}

int8_t step_to_accuracy_decimals(float step) {
  // use printf %g to find number of digits based on temperature step
  std::string str = str_sprintf("%.5g", step);
  size_t dot_pos = str.find('.');
  if (dot_pos == std::string::npos)
    return 0;

  return str.length() - dot_pos - 1;
}

// wrapper around std::transform to run safely on functions from the ctype.h header
// see https://en.cppreference.com/w/cpp/string/byte/toupper#Notes
template<int (*fn)(int)> std::string str_ctype_transform(const std::string &str) {
  std::string result;
  result.resize(str.length());
  std::transform(str.begin(), str.end(), result.begin(), [](unsigned char ch) { return fn(ch); });
  return result;
}
std::string str_lower_case(const std::string &str) { return str_ctype_transform<std::tolower>(str); }
std::string str_upper_case(const std::string &str) { return str_ctype_transform<std::toupper>(str); }

size_t parse_hex(const char *str, size_t length, uint8_t *data, size_t count) {
  uint8_t val;
  size_t chars = std::min(length, 2 * count);
  for (size_t i = 2 * count - chars; i < 2 * count; i++, str++) {
    if (*str >= '0' && *str <= '9') {
      val = *str - '0';
    } else if (*str >= 'A' && *str <= 'F') {
      val = 10 + (*str - 'A');
    } else if (*str >= 'a' && *str <= 'f') {
      val = 10 + (*str - 'a');
    } else {
      return 0;
    }
    data[i >> 1] = !(i & 1) ? val << 4 : data[i >> 1] | val;
  }
  return chars;
}

}  // namespace esphome
