#include "harness.h"

#include "tion-api-4s.h"
#include "tion-api-uart-4s.h"

using dentra::tion::Tion4sUartProtocol;
using dentra::tion::TionGatePosition;
using dentra::tion::TionState;
using dentra::tion::TionStateCall;
using dentra::tion_4s::tion4s_raw_frame_t;
using dentra::tion_4s::tion4s_state_set_t;
using dentra::tion_4s::tion4s_state_t;
using dentra::tion_4s::Tion4sApi;

using namespace dentra::tion_4s;

namespace {

using Harness4s = test::UartHarness<Tion4sApi, Tion4sUartProtocol>;

tion4s_state_t make_state() {
  tion4s_state_t st{};
  st.power_state = true;
  st.sound_state = true;
  st.led_state = false;
  st.heater_state = true;
  st.heater_mode = tion4s_state_t::HEATER_MODE_HEATING;
  st.heater_present = tion4s_state_t::HEATER_PRESENT_1400W;
  st.gate_position = tion4s_state_t::GATE_POSITION_OUTDOOR;
  st.target_temperature = 18;
  st.fan_speed = 3;
  st.outdoor_temperature = -5;
  st.current_temperature = 17;
  return st;
}

test::bytes_t state_response(const tion4s_state_t &st, uint32_t request_id = 1) {
  return test::encode_uart<Tion4sUartProtocol>(FRAME_TYPE_STATE_RSP, tion4s_raw_frame_t<tion4s_state_t>{request_id, st});
}

}  // namespace

TEST_CASE("4s uart: requests encoding") {
  Harness4s h;

  SUBCASE("first request_state also requests device info") {
    h.api.request_state();
    CHECK(h.take_tx() == test::hex("3A.07.00.32.33.6F.A6 3A.07.00.32.32.7F.87"));
  }
  SUBCASE("send_heartbeat") {
    h.api.send_heartbeat();
    CHECK(h.take_tx() == test::hex("3A.07.00.32.39.CE.EC"));
  }
  SUBCASE("request_time") {
    h.api.request_time(0);
    CHECK(h.take_tx() == test::hex("3A.0B.00.32.36.00.00.00.00.B0.E0"));
  }
}

TEST_CASE("4s uart: device info response") {
  Harness4s h;
  dentra::tion::tion_dev_info_t info{};
  info.work_mode = dentra::tion::tion_dev_info_t::NORMAL;
  info.device_type = dentra::tion::tion_dev_info_t::BR4S;
  info.firmware_version = 0x0230;
  info.hardware_version = 0x0100;

  h.receive(test::encode_uart<Tion4sUartProtocol>(FRAME_TYPE_DEV_INFO_RSP, info));

  CHECK(h.api.get_state().firmware_version == 0x0230);
  CHECK(h.api.get_state().hardware_version == 0x0100);

  // device info is known, only state is requested
  h.api.request_state();
  CHECK(h.take_tx() == test::hex("3A.07.00.32.32.7F.87"));
}

TEST_CASE("4s uart: heartbeat response") {
  Harness4s h;
  int work_mode = -1;
  h.api.set_on_heartbeat([&](uint8_t wm) { work_mode = wm; });

  h.receive("3A.08.00.31.39.00.E8.2B");

  CHECK(h.rx_frames == 1);
  CHECK(work_mode == 0);
}

TEST_CASE("4s uart: corrupted frames are ignored") {
  Harness4s h;
  int work_mode = -1;
  h.api.set_on_heartbeat([&](uint8_t wm) { work_mode = wm; });

  SUBCASE("bad crc") { h.receive("3A.08.00.31.39.00.E8.2C"); }
  SUBCASE("bad size") { h.receive("3A.AA.00.31.39.00.E8.2B"); }

  CHECK(work_mode == -1);
}

TEST_CASE("4s uart: state response updates api state") {
  Harness4s h;
  int notified = 0;
  h.api.set_on_state([&](const TionState &, uint32_t) { notified++; });

  h.receive(state_response(make_state()));

  REQUIRE(h.rx_frames == 1);
  CHECK(notified == 1);
  const auto &st = h.api.get_state();
  CHECK(st.power_state);
  CHECK(st.sound_state);
  CHECK_FALSE(st.led_state);
  CHECK(st.heater_state);
  CHECK(st.gate_position == TionGatePosition::OUTDOOR);
  CHECK(st.target_temperature == 18);
  CHECK(st.fan_speed == 3);
  CHECK(st.outdoor_temperature == -5);
  CHECK(st.current_temperature == 17);
}

TEST_CASE("4s uart: state call writes state set frame") {
  Harness4s h;
  h.receive(state_response(make_state()));
  h.tx.clear();

  TionStateCall call(&h.api);
  call.set_fan_speed(5);
  call.set_target_temperature(20);
  call.set_heater_state(false);
  call.perform();

  const auto frames = test::decode_uart<Tion4sUartProtocol>(h.tx);
  REQUIRE(frames.size() == 1);
  CHECK(frames[0].type == FRAME_TYPE_STATE_SET);
  const auto &req = frames[0].as<tion4s_raw_frame_t<tion4s_state_set_t>>();
  CHECK(req.data.power_state);
  CHECK(req.data.sound_state);
  CHECK(req.data.fan_speed == 5);
  CHECK(req.data.target_temperature == 20);
  CHECK(req.data.heater_mode == tion4s_state_t::HEATER_MODE_FANONLY);
  CHECK(req.data.gate_position == tion4s_state_t::GATE_POSITION_OUTDOOR);
}

TEST_CASE("4s uart: state call without changes writes nothing") {
  Harness4s h;
  h.receive(state_response(make_state()));
  h.tx.clear();

  TionStateCall call(&h.api);
  call.perform();

  CHECK(h.tx.empty());
}
