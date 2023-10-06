#include <cstring>
#include <cstdlib>
#include <stdio.h>
#include <string>
#include <vector>
#include <cstddef>
#include <random>
#include <ctime>
#include <chrono>
#include <iostream>
#include <new>
#include <vector>

#include "utils.h"

#include "../components/tion-api/crc.h"

DEFINE_TAG;

#define NUM_REPEATS 3

using namespace dentra::tion;

using test_fn_t = std::function<bool(bool)>;

using crc_fn_t = std::function<uint16_t(const void *, uint16_t)>;

bool check_crc(crc_fn_t &&crc_fn, bool print) {
  uint8_t buf[32];
  for (int i = 0; i < sizeof(buf); i++) {
    buf[i] = fast_random_8();
  }
  if (print)
    printf("src: %s\n", hexencode_cstr(&buf, sizeof(buf)));

  uint16_t crc = __builtin_bswap16(crc_fn(buf, sizeof(buf)));
  if (print)
    printf("crc: %04x\n", crc);

  char bufc[sizeof(buf) + sizeof(crc)];
  std::memcpy(bufc, &buf, sizeof(bufc));
  std::memcpy(bufc + sizeof(buf), &crc, sizeof(crc));
  if (print)
    printf("chk: %s\n", hexencode_cstr(&bufc, sizeof(bufc)));

  crc = crc_fn(bufc, sizeof(bufc));
  if (print)
    printf("res: %x\n", crc);
  return crc == 0;
}

bool mk_test(test_fn_t &&func) {
  bool res = true;
  using namespace std::chrono;
  high_resolution_clock::time_point t1 = high_resolution_clock::now();

  for (int i = 0; i < NUM_REPEATS; i++) {
    if (!func(false)) {
      res = false;
    }
  }

  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double, std::milli> time_span = t2 - t1;

  printf("%lf\n", time_span.count());

  return res;
}

// last 2 bytes must contain CRC
bool test_crc_data(const char *data) {
  bool res = true;

  auto tx_frame = cloak::from_hex(data);

  uint16_t crc = __builtin_bswap16(crc16_ccitt_false(tx_frame.data(), tx_frame.size() - 2));
  uint16_t chk = *((uint16_t *) (tx_frame.data() + tx_frame.size() - 2));
  res &= cloak::check_data_(crc, chk);

  uint16_t zer = crc16_ccitt_false(tx_frame.data(), tx_frame.size());
  res &= cloak::check_data_(zer, 0);

  return res;
}

bool test_api_crc() {
  bool res = true;
  // res = mk_test([&](bool) { return check_crc(crc16_ccitt_false, print); });

  res &= test_crc_data("0C.00.3A.AD.32.33.01.00.00.00.CE.A6");
  res &= test_crc_data("0C.00.3A.AD.32.32.01.00.00.00.64.F7");
  res &=
      test_crc_data("2F.00.3A.D1.31.32.01.00.00.00.00.00.00.00.3C.51.00.10.01.0C.17.12.1E.71.EF.29.00.D8.16.1F.00.28."
                    "37.CE.00.FE.56.43.00.00.00.00.00.06.00.63.1A");
  res &= test_crc_data(
      "25.00.3A.20.31.33.01.00.00.00.01.03.80.00.00.BC.02.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.05.23");

  return res;
}

REGISTER_TEST(test_api_crc);
