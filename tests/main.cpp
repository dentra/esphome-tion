#include <cstring>
#include <cstdlib>
#include <stdio.h>
#include <string>
#include <vector>
#include "inttypes.h"
#include "byteswap.h"
#include <random>
#include <ctime>
#include <chrono>
#include <functional>
#include <cstddef>
#include <iostream>
#include <new>
#include <vector>
#include <execinfo.h>

#include "utils.h"
#include "../components/tion-api/log.h"

#include "test_api.h"
#include "test_api_4s.h"
#include "test_api_lt.h"
#include "test_api_crc.h"
#include "test_api_3s.h"

bool test_cl(bool print);

int main(int argc, char const *argv[]) {
  dentra::tion::set_logger(printf_logger);

  auto tests = {
      &test_api_crc, &test_api, &test_api_4s, &test_api_lt, &test_api_3s,
  };
  bool res = true;
  for (auto test : tests) {
    backtrace_symbols_fd((void *const *) &test, 1, 1);
    if (!test(true)) {
      res = false;
    }
  }

  if (res) {
    LOGI("SUCCESS");
  } else {
    LOGE("FAILED");
  }

  return res ? 0 : 1;
}
