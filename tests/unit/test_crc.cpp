#include "harness.h"

#include "crc.h"

using dentra::tion::crc16_ccitt_false_ffff;

namespace {

// last 2 bytes of a frame contain big-endian crc
void check_frame_crc(const std::string &frame_hex) {
  const auto frame = test::from_hex(frame_hex);
  REQUIRE(frame.size() > 2);
  const uint16_t crc = crc16_ccitt_false_ffff(frame.data(), frame.size() - 2);
  CHECK(crc == (frame[frame.size() - 2] << 8 | frame[frame.size() - 1]));
  // crc over a frame with its crc is always zero
  CHECK(crc16_ccitt_false_ffff(frame.data(), frame.size()) == 0);
}

}  // namespace

TEST_CASE("crc16 ccitt-false of real frames") {
  check_frame_crc("0C.00.3A.AD.32.33.01.00.00.00.CE.A6");
  check_frame_crc("0C.00.3A.AD.32.32.01.00.00.00.64.F7");
  check_frame_crc("2F.00.3A.D1.31.32.01.00.00.00.00.00.00.00.3C.51.00.10.01.0C.17.12.1E.71.EF.29.00.D8.16.1F.00.28."
                  "37.CE.00.FE.56.43.00.00.00.00.00.06.00.63.1A");
  check_frame_crc("25.00.3A.20.31.33.01.00.00.00.01.03.80.00.00.BC.02.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00."
                  "00.00.00.05.23");
  check_frame_crc("3A.07.00.32.32.7F.87");
  check_frame_crc("3A.07.00.32.39.CE.EC");
}
