#include "harness.h"

#include "tion-api-o2.h"
#include "tion-api-uart-o2.h"

using dentra::tion::TionStateCall;
using dentra::tion_o2::TionO2Api;
using dentra::tion_o2::TionO2UartProtocol;
using dentra::tion_o2::tiono2_state_set_t;

using namespace dentra::tion_o2;

namespace {

using HarnessO2 = test::UartHarness<TionO2Api, TionO2UartProtocol>;

// real state dumps
// power off, heater on, outdoor 12°C, current 19°C, target 16°C, speed 2, 60 m3/h
const char *const STATE_OFF = "11 0C 0C 13 10 02 3C 04 00 00 B4 D6 DC 01 01 F6 CA 01 54";
// power on, heater on, outdoor 3°C, current 9°C, target 10°C, speed 1, 35 m3/h
const char *const STATE_ON = "11 0E 03 09 0A 01 23 04 00 00 C0 AA DE 01 D3 20 C9 01 48";

}  // namespace

TEST_CASE("o2 uart: first request_state connects and requests device info") {
  HarnessO2 h;
  h.api.request_state();
  const auto frames = test::decode_uart<TionO2UartProtocol>(test::from_hex(h.take_tx()), true);
  REQUIRE(frames.size() == 4);
  CHECK(frames[0].type == FRAME_TYPE_CONNECT_REQ);
  CHECK(frames[1].type == FRAME_TYPE_DEV_INFO_REQ);
  CHECK(frames[2].type == FRAME_TYPE_DEV_MODE_REQ);
  CHECK(frames[3].type == FRAME_TYPE_STATE_GET_REQ);
}

TEST_CASE("o2 uart: real state responses") {
  HarnessO2 h;

  SUBCASE("power off") {
    h.receive(STATE_OFF);
    REQUIRE(h.rx_frames == 1);
    const auto &st = h.api.get_state();
    CHECK_FALSE(st.power_state);
    CHECK(st.heater_state);
    CHECK(st.outdoor_temperature == 12);
    CHECK(st.current_temperature == 19);
    CHECK(st.target_temperature == 16);
    CHECK(st.fan_speed == 2);
    CHECK(st.productivity == 60);
  }

  SUBCASE("power on") {
    h.receive(STATE_ON);
    REQUIRE(h.rx_frames == 1);
    const auto &st = h.api.get_state();
    CHECK(st.power_state);
    CHECK(st.heater_state);
    CHECK(st.outdoor_temperature == 3);
    CHECK(st.current_temperature == 9);
    CHECK(st.target_temperature == 10);
    CHECK(st.fan_speed == 1);
    CHECK(st.productivity == 35);
  }
}

TEST_CASE("o2 uart: corrupted state is ignored") {
  HarnessO2 h;
  // last byte is crc
  h.receive("11 0E 03 09 0A 01 23 04 00 00 C0 AA DE 01 D3 20 C9 01 49");
  CHECK(h.rx_frames == 0);
}

TEST_CASE("o2 uart: state call writes state set frame") {
  HarnessO2 h;
  h.receive(STATE_ON);
  h.tx.clear();

  TionStateCall call(&h.api);
  call.set_fan_speed(3);
  call.set_target_temperature(15);
  call.perform();

  const auto frames = test::decode_uart<TionO2UartProtocol>(h.tx, true);
  REQUIRE(frames.size() >= 1);
  CHECK(frames[0].type == FRAME_TYPE_STATE_SET_REQ);
  const auto &req = frames[0].as<tiono2_state_set_t>();
  CHECK(req.fan_speed == 3);
  CHECK(req.target_temperature == 15);
  CHECK(req.power_state);
  CHECK(req.heater_state);
}

TEST_CASE("o2 uart: state is not written before first state response") {
  HarnessO2 h;

  TionStateCall call(&h.api);
  call.set_power_state(false);
  call.perform();

  CHECK(h.tx.empty());
}
