#include "harness.h"

#include "tion-api-3s.h"
#include "tion-api-uart-3s.h"

using dentra::tion::Tion3sUartProtocol;
using dentra::tion::TionGatePosition;
using dentra::tion::TionStateCall;
using dentra::tion::Tion3sApi;

namespace {

using Harness3s = test::UartHarness<Tion3sApi, Tion3sUartProtocol>;

// real state dump: speed 1, gate outdoor, 23°C, heater+power+sound on, filter 79 days, 14:45
const char *const REAL_STATE = "B3.10 21.17.0B.00.00.00.00.4F.00.0E.2D.00.00.00.00.FF.FF 5A";

}  // namespace

TEST_CASE("3s uart: request_state") {
  Harness3s h;
  h.api.request_state();
  CHECK(h.take_tx() == test::hex("3D.01 00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00 5A"));
}

TEST_CASE("3s uart: real state response") {
  Harness3s h;
  int notified = 0;
  h.api.set_on_state([&](const dentra::tion::TionState &, uint32_t) { notified++; });

  h.receive(REAL_STATE);

  REQUIRE(h.rx_frames == 1);
  CHECK(notified == 1);
  const auto &st = h.api.get_state();
  CHECK(st.fan_speed == 1);
  CHECK(st.gate_position == TionGatePosition::OUTDOOR);
  CHECK(st.target_temperature == 23);
  CHECK(st.heater_state);
  CHECK(st.power_state);
  CHECK(st.sound_state);
  CHECK(st.firmware_version == 0xFFFF);
}

TEST_CASE("3s uart: frame split across reads") {
  Harness3s h;
  const auto data = test::from_hex(REAL_STATE);
  h.receive(test::bytes_t(data.begin(), data.begin() + 7));
  CHECK(h.rx_frames == 0);
  h.receive(test::bytes_t(data.begin() + 7, data.end()));
  CHECK(h.rx_frames == 1);
}

TEST_CASE("3s uart: garbage before frame is skipped") {
  Harness3s h;
  h.receive("00.FF.12");
  h.receive(REAL_STATE);
  CHECK(h.rx_frames == 1);
  CHECK(h.api.get_state().target_temperature == 23);
}

TEST_CASE("3s uart: state call writes full state") {
  Harness3s h;
  h.receive(REAL_STATE);
  h.tx.clear();

  TionStateCall call(&h.api);
  call.set_fan_speed(4);
  call.perform();

  //                       fan tmp gate flags      filter      reset srv
  CHECK(h.take_tx() == test::hex("3D.02 04.17.02.0B.01.00.00.00.00.00.00.00.00.00.00.00.00 5A"));
}

TEST_CASE("3s uart: state is not written before first state response") {
  Harness3s h;

  TionStateCall call(&h.api);
  call.set_fan_speed(4);
  call.perform();

  CHECK(h.tx.empty());
}

TEST_CASE("3s uart: reset filter") {
  Harness3s h;
  h.receive(REAL_STATE);
  h.tx.clear();

  h.api.reset_filter();

  CHECK(h.take_tx() == test::hex("3D.02 01.17.02.0B.01.02.00.00.00.00.00.00.00.00.00.00.00 5A"));
}

TEST_CASE("3s uart: stale state does not cancel boost") {
  Harness3s h;
  test::FakeClock::set(100000);
  h.receive(REAL_STATE);
  h.api.set_boost_time(60);
  h.tx.clear();

  TionStateCall call(&h.api);
  static_cast<dentra::tion::TionApiBase &>(h.api).enable_boost(true, &call);
  call.perform();
  CHECK(h.take_tx() == test::hex("3D.02 06.17.02.0B.01.00.00.00.00.00.00.00.00.00.00.00.00 5A"));

  // stale state with speed 1, sent before boost was applied
  h.receive(REAL_STATE);
  CHECK(h.api.is_boost_running());
  CHECK(h.tx.empty());

  // boost applied: speed 6
  h.receive("B3.10 26.17.0B.00.00.00.00.4F.00.0E.2D.00.00.00.00.FF.FF 5A");
  CHECK(h.api.is_boost_running());

  // boost time is over, state is restored
  test::FakeClock::advance(61 * 1000);
  h.receive("B3.10 26.17.0B.00.00.00.00.4F.00.0E.2D.00.00.00.00.FF.FF 5A");
  CHECK_FALSE(h.api.is_boost_running());
  CHECK(h.take_tx() == test::hex("3D.02 01.17.02.0B.01.00.00.00.00.00.00.00.00.00.00.00.00 5A"));
}
