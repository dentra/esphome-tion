#include "harness.h"

#include "tion-api-lt.h"
#include "tion-api-uart-lt.h"

using dentra::tion::TionLtApi;
using dentra::tion::TionStateCall;
using dentra::tion_lt::TionLtUartProtocol;

namespace {

using HarnessLtUart = test::UartHarness<TionLtApi, TionLtUartProtocol>;

std::string text(const test::bytes_t &data) { return std::string(data.begin(), data.end()); }

// real console output of getstate command
const char *const REAL_STATE = "\r\n"
                               "Switching Mode\r\n"
                               "Current Mode: Work\r\n"
                               "Speed: 2\r\n"
                               "Sensors T_set: 12, T_In: -23, T_out: 24\r\n"
                               "PID_Value: 17 1\r\n"
                               "Filter Time: 10984449\r\n"
                               "Working Time: 27853548\r\n"
                               "Power On Time: 81257423\r\n"
                               "Error register: 255\r\n"
                               "MAC: 247 74 249 223 116 211\r\n"
                               "Firmware Version 0x054B\r\n"
                               "\r\n";

void receive_text(HarnessLtUart &h, const char *str) {
  h.receive(test::bytes_t(reinterpret_cast<const uint8_t *>(str), reinterpret_cast<const uint8_t *>(str) + strlen(str)));
}

}  // namespace

TEST_CASE("lt uart: first request_state enables console and requests state") {
  HarnessLtUart h;
  h.api.request_state();
  CHECK(text(h.tx) == "logenable noit\r\ngetstate\r\n");
}

TEST_CASE("lt uart: real console state") {
  HarnessLtUart h;
  receive_text(h, REAL_STATE);

  REQUIRE(h.rx_frames >= 1);
  const auto &st = h.api.get_state();
  CHECK(st.power_state);
  CHECK(st.fan_speed == 2);
  CHECK(st.target_temperature == 12);
  CHECK(st.outdoor_temperature == -23);
  CHECK(st.current_temperature == 24);
  CHECK(st.firmware_version == 0x054B);
  CHECK(st.fan_time == 27853548);
}

TEST_CASE("lt uart: state call sends only changed parameters") {
  HarnessLtUart h;
  receive_text(h, REAL_STATE);
  h.tx.clear();

  TionStateCall call(&h.api);
  call.set_fan_speed(4);
  call.perform();

  CHECK(text(h.tx) == "set_speed 4\r\ngetstate\r\n");
}

TEST_CASE("lt uart: power off") {
  HarnessLtUart h;
  receive_text(h, REAL_STATE);
  h.tx.clear();

  TionStateCall call(&h.api);
  call.set_power_state(false);
  call.perform();

  CHECK(text(h.tx) == "stby\r\ngetstate\r\n");
}
