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
  char buf[32];
  sprintf(buf, "%.5g", step);

  std::string str{buf};
  size_t dot_pos = str.find('.');
  if (dot_pos == std::string::npos)
    return 0;

  return str.length() - dot_pos - 1;
}

}  // namespace esphome
