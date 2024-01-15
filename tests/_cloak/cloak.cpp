#include <map>
#include <set>

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/components/logger/logger.h"

#include "cloak.h"

DEFINE_TAG;

using namespace esphome;

int main(int argc, char const *argv[]) __attribute__((weak));
int main(int argc, char const *argv[]) { return cloak::run_tests(argc, argv); }

namespace cloak {

struct TestKit {
  std::map<test_fn_t, std::string> tests{};
};
TestKit *kit{};

void register_test(const std::string &name, test_fn_t fn) {
  printf("Registering %s\n", name.c_str());
  if (kit == nullptr) {
    kit = new TestKit;
  }
  kit->tests.emplace(fn, name);
}

logger::Logger logger(0, 0, logger::UART_SELECTION_UART0);
bool run_tests(const std::set<std::string> &run_only) {
  logger.pre_setup();

  std::vector<std::string> failed_tests;

  for (const auto &[test, name] : kit->tests) {
    if (run_only.empty() || run_only.find(name) != run_only.end()) {
      printf("****************************************\n\n");
      printf("    %s\n\n", name.c_str());
      printf("****************************************\n");
      if (!test()) {
        failed_tests.push_back(name);
      }
    }
  }
  if (failed_tests.empty()) {
    ESP_LOGI(TAG, "SUCCESS");
  } else {
    ESP_LOGE(TAG, "FAILED");
    for (auto name : failed_tests) {
      ESP_LOGE(TAG, "  %s", name.c_str());
    }
  }
  return failed_tests.empty();
}

int run_tests(int argc, char const *argv[]) {
  std::set<std::string> run_only;
  for (int i = 1; i < argc; i++) {
    const char *s = argv[i];
    if (*s == '-') {
      ++s;
      printf("Excluded %s\n", s);
      for (auto i = kit->tests.begin(), last = kit->tests.end(); i != last;) {
        if (i->second == s) {
          i = kit->tests.erase(i);
        } else {
          ++i;
        }
      }
    } else {
      run_only.insert(s);
      printf("Run only %s\n", s);
    }
  }
  return run_tests(run_only) ? 0 : 1;
}

// converts hex string to bytes skip spaces, dots and dashes if exist.
std::vector<uint8_t> from_hex(const std::string &hex) {
  std::vector<uint8_t> res;
  for (const char *ptr = hex.c_str(), *end = ptr + hex.length(); ptr < end; ptr++) {
    if (*ptr == ' ' || *ptr == '.' || *ptr == '-' || *ptr == ':') {
      continue;
    }
    auto byte = internal::char2int(*ptr) << 4;
    ptr++;
    byte += internal::char2int(*ptr);
    res.push_back(byte);
  }
  return res;
}

#define print_data1(TAG, expr, name, fmt, act, exp) \
  bool res = expr; \
  if (res) { \
    ESP_LOGI(TAG, "test %s: SUCCESS", name.c_str()); \
    ESP_LOGD(TAG, "  data: " fmt, act); \
  } else { \
    ESP_LOGE(TAG, "test %s: FAILED", name.c_str()); \
    ESP_LOGE(TAG, "  act: " fmt, act); \
    ESP_LOGE(TAG, "  exp: " fmt, exp); \
  } \
  return res

#define print_data2(TAG, expr, name, fmt1, fmt2, act, exp) \
  bool res = expr; \
  if (res) { \
    ESP_LOGI(TAG, "test %s: SUCCESS", name.c_str()); \
    ESP_LOGD(TAG, "  data: " fmt1 " (" fmt2 ")", act, act); \
  } else { \
    ESP_LOGE(TAG, "test %s: FAILED", name.c_str()); \
    ESP_LOGE(TAG, "  act: " fmt1 " (" fmt2 ")", act, act); \
    ESP_LOGE(TAG, "  exp: " fmt1 " (" fmt2 ")", exp, exp); \
  } \
  return res

namespace internal {
int char2int(char ch) {
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

bool test(const char *tag, const std::string &name, bool data1, bool data2) {
  print_data1(tag, data1 == data2, name, "%s", TRUEFALSE(data1), TRUEFALSE(data2));
}

bool test(const char *tag, const std::string &name, uint32_t data1, uint32_t data2) {
  print_data2(tag, data1 == data2, name, "%u", "0x%X", data1, data2);
}

bool test(const char *tag, const std::string &name, int32_t data1, int32_t data2) {
  print_data2(tag, data1 == data2, name, "%d", "0x%X", data1, data2);
}

bool test(const char *tag, const std::string &name, double data1, double data2) {
  print_data1(tag, data1 == data2, name, "%lf", data1, data2);
}

bool test(const char *tag, const std::string &name, const std::string data1, const std::string data2) {
  printf("checking string %s == %s, %u\n", data1.c_str(), data2.c_str(), data1 == data2);
  printf("%zu, %zu\n", data1.size(), data2.size());
  print_data1(tag, data1 == data2, name, "%s", data1.c_str(), data2.c_str());
}

bool test(const char *tag, const std::string &name, const std::vector<uint8_t> &data1,
          const std::vector<uint8_t> &data2) {
  print_data1(tag, data1 == data2, name, "%s", format_hex_pretty(data1).c_str(), format_hex_pretty(data2).c_str());
}
}  // namespace internal
}  // namespace cloak
