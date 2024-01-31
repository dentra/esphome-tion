
#include <cstring>
#include <cstdint>
#include <optional>
#include <cinttypes>
#include "utils.h"
#include "../components/nvs/nvs.h"

#include "http_parser.h"

DEFINE_TAG;

using namespace esphome::nvs_flash;

bool test_nvs() {
  bool res = true;

  NvsFlash nvs("test");

  int32_t i32 = 0xFFFFFFFF;
  nvs.set("i32", i32);
  i32 = *nvs.get<int32_t>("i32");
  res &= cloak::check_data("i32", i32, -1);

  unsigned int _uint = 0xFFFFFFFF;
  nvs.set("_uint", _uint);
  res &= cloak::check_data("_uint", *nvs.get<unsigned int>("_uint"), _uint);

  float _float = 4294967295;
  nvs.set("_float", _float);
  res &= cloak::check_data("_float", *nvs.get<float>("_float"), _float);

  double _double = _float;
  nvs.set("_double", _double);
  res &= cloak::check_data("_double", *nvs.get<double>("_double"), _double);

  std::string _string = "abcd";
  nvs.set("_string", _string);
  res &= cloak::check_data("_string", *nvs.get<std::string>("_string"), _string);

  struct X {
    void set_address(uint64_t x) { printf("OK %llu\n", x); }
  } _tion_ble_client;

  auto tion_ble_client = &_tion_ble_client;

  uint64_t tion_mac_address = -1ULL;
  nvs.set("tion_mac_address", tion_mac_address);

  auto sav = nvs.get<uint64_t>("tion_mac_address");
  if (sav.has_value()) {
    [&](uint64_t x) { tion_ble_client->set_address(x); }(sav.value());
  }

  return res;
}

using esphome::parse_hex;

void url_decode(char *str) {
  char *ptr = str, buf;
  for (; *str; str++, ptr++) {
    if (*str == '%') {
      str++;
      if (parse_hex(str, 2, reinterpret_cast<uint8_t *>(&buf), 1) == 2) {
        *ptr = buf;
        str++;
      } else {
        str--;
        *ptr = *str;
      }
    } else if (*str == '+') {
      *ptr = ' ';
    } else {
      *ptr = *str;
    }
  }
  *ptr = *str;
}

const char buf[] = "/?param0=a+b";

std::optional<std::string> get_opt(bool x) {
  if (x)
    return {buf};
  return {};
}

void enum_errors(uint32_t errors, const std::function<void(const std::string &)> &fn) {
  if (errors == 0) {
    return;
  }
  for (uint32_t i = 0; i <= 10; i++) {
    uint32_t mask = 1 << i;
    if ((errors & mask) == mask) {
      fn(esphome::str_snprintf("EC%02" PRIu32, 4, i + 1));
    }
  }
  for (uint32_t i = 24; i <= 29; i++) {
    uint32_t mask = 1 << i;
    if ((errors & mask) == mask) {
      fn(esphome::str_snprintf("WS%02" PRIu32, 4, i + 1));
    }
  }
}
bool test_parser() {
  bool res = true;

  std::string s;

  s = "https%3A%2F%2FHello%2BWorld%2B%253E%2Bhow%2Bare%2Byou%253F";
  url_decode(s.data());
  s = s.c_str();
  printf("%s\n", s.c_str());
  res &= s == "https://Hello+World+%3E+how+are+you%3F";

  s = "https%3A%2F%2FHello+World%2B%253E%2Bhow%2Bare%2Byou%253F";
  url_decode(s.data());
  s = s.c_str();
  printf("%s\n", s.c_str());
  res &= s == "https://Hello World+%3E+how+are+you%3F";

  s = "%3N%2F+%%+%+%2F%3";
  url_decode(s.data());
  s = s.c_str();
  printf("%s\n", s.c_str());
  res &= s == "%3N/ %% % /%3";

  s = "https://ru.wikipedia.org/wiki/%D0%A2%D1%80%D0%B0%D0%BD%D1%81%D0%BF%D0%B0%D0%B9%D0%BB%D0%B5%D1%80";
  url_decode(s.data());
  s = s.c_str();
  printf("%s\n", s.c_str());
  res &= s == "https://ru.wikipedia.org/wiki/Транспайлер";

  s = "%3D%3F%25%26%2A%40";
  url_decode(s.data());
  s = s.c_str();
  printf("%s\n", s.c_str());
  res &= s == "=?%&*@";

  s = "%D0%BC%D0%B0%D0%BC%D0%B0%20%D0%BC%D1%8B%D0%BB%D0%B0%20%D1%80%D0%B0%D0%BC%D1%83";
  url_decode(s.data());
  s = s.c_str();
  printf("%s\n", s.c_str());
  res &= s == "мама мыла раму";

  std::string codes;
  enum_errors(1 << 0 | 1 << 28, [&codes](auto code) { codes += (codes.empty() ? "" : ", ") + code; });
  printf("%s\n", codes.c_str());

  return res;
}

REGISTER_TEST(test_nvs);
REGISTER_TEST(test_parser);
