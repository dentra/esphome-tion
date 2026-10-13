// Optional features: antifreeze, zero fan speed (kiv), lt button presets.
#include "harness.h"

#include "tion-api-3s.h"
#include "tion-api-lt.h"
#include "tion-api-uart-3s.h"
#include "tion-api-uart-lt.h"

using dentra::tion::Tion3sApi;
using dentra::tion::Tion3sUartProtocol;
using dentra::tion::TionLtApi;
using dentra::tion::TionStateCall;
using dentra::tion_lt::button_presets_t;
using dentra::tion_lt::TionLtUartProtocol;
using dentra::tion_lt::tionlt_state_get_req_t;
using dentra::tion_lt::tionlt_state_set_req_t;

namespace {

using Harness3s = test::UartHarness<Tion3sApi, Tion3sUartProtocol>;
using HarnessLtUart = test::UartHarness<TionLtApi, TionLtUartProtocol>;

std::string text(const test::bytes_t &data) { return std::string(data.begin(), data.end()); }

void receive_text(HarnessLtUart &h, const std::string &str) { h.receive(test::bytes_t(str.begin(), str.end())); }

/// lt console state, speed 2, power on
std::string lt_state(int outdoor, bool heater) {
  char buf[512];
  snprintf(buf, sizeof(buf),
           "\r\n"
           "Switching Mode\r\n"
           "Current Mode: Work\r\n"
           "Speed: 2\r\n"
           "Sensors T_set: 12, T_In: %d, T_out: 24\r\n"
           "PID_Value: 17 %d\r\n"
           "Filter Time: 10984449\r\n"
           "Working Time: 27853548\r\n"
           "Power On Time: 81257423\r\n"
           "Error register: 0\r\n"
           "MAC: 247 74 249 223 116 211\r\n"
           "Firmware Version 0x054B\r\n"
           "\r\n",
           outdoor, heater ? 1 : 0);
  return buf;
}

/// 3s state with given flags and outdoor temperature: speed 1, gate outdoor, 23°C.
std::string state_3s(uint8_t flags, int8_t outdoor) {
  char buf[128];
  snprintf(buf, sizeof(buf), "B3.10 21.17.%02X.00.00.00.%02X.4F.00.0E.2D.00.00.00.00.FF.FF 5A", flags,
           static_cast<uint8_t>(outdoor));
  return buf;
}

// 3s flags: bit 0 heater, bit 1 power, bit 3 sound
constexpr uint8_t FLAGS_POWER = 0x0A;
constexpr uint8_t FLAGS_POWER_HEATER = 0x0B;
constexpr uint8_t FLAGS_OFF = 0x08;

}  // namespace

TEST_CASE("antifreeze: heater is enabled on frost") {
  Harness3s h;
  h.receive(state_3s(FLAGS_POWER, -5));
  CHECK(h.take_tx() == test::hex("3D.02 01.17.02.0B.01.00.00.00.00.00.00.00.00.00.00.00.00 5A"));
}

TEST_CASE("antifreeze: no action when not needed") {
  Harness3s h;
  SUBCASE("positive outdoor temperature") { h.receive(state_3s(FLAGS_POWER, 5)); }
  SUBCASE("zero outdoor temperature") { h.receive(state_3s(FLAGS_POWER, 0)); }
  SUBCASE("heater already on") { h.receive(state_3s(FLAGS_POWER_HEATER, -5)); }
  SUBCASE("power off") { h.receive(state_3s(FLAGS_OFF, -5)); }
  REQUIRE(h.rx_frames == 1);
  CHECK(h.tx.empty());
}

TEST_CASE("kiv: zero fan speed without kiv support turns breezer off") {
  HarnessLtUart h;
  receive_text(h, lt_state(10, false));
  h.tx.clear();

  TionStateCall call(&h.api);
  call.set_fan_speed(0);
  call.perform();

  CHECK(text(h.tx) == "stby\r\ngetstate\r\n");
}

TEST_CASE("kiv: zero fan speed with kiv support keeps breezer on") {
  HarnessLtUart h;
  h.api.enable_kiv_support();
  receive_text(h, lt_state(10, false));
  h.tx.clear();

  TionStateCall call(&h.api);
  call.set_fan_speed(0);
  call.perform();

  CHECK(text(h.tx) == "set_speed 0\r\ngetstate\r\n");
}

TEST_CASE("kiv: zero fan speed is not allowed on frost or with heater") {
  HarnessLtUart h;
  h.api.enable_kiv_support();
  SUBCASE("outdoor temperature less than 5") { receive_text(h, lt_state(4, false)); }
  SUBCASE("heater is on") { receive_text(h, lt_state(10, true)); }
  h.tx.clear();

  TionStateCall call(&h.api);
  call.set_fan_speed(0);
  call.perform();

  // breezer is turned off instead, current speed is kept
  CHECK(text(h.tx) == "stby\r\ngetstate\r\n");
}

TEST_CASE("lt: button presets are sent with state") {
  TionLtApi api;
  std::vector<test::Frame> frames;
  api.set_api_writer([&frames](uint16_t type, const void *data, size_t size) {
    const auto *p = static_cast<const uint8_t *>(data);
    frames.push_back({type, test::bytes_t(p, p + size)});
    return true;
  });

  tionlt_state_get_req_t rsp{};
  rsp.request_id = 1;
  rsp.state.power_state = true;
  rsp.state.fan_speed = 2;
  rsp.state.target_temperature = 16;
  rsp.state.button_presets = {.tmp{1, 2, 3}, .fan{1, 2, 3}};
  api.read_frame(dentra::tion_lt::FRAME_TYPE_STATE_RSP, &rsp, sizeof(rsp));
  REQUIRE(api.get_state().fan_speed == 2);

  api.set_button_presets({.tmp{10, 15, 20}, .fan{1, 3, 5}});
  frames.clear();

  TionStateCall call(&api);
  call.set_fan_speed(4);
  call.perform();

  REQUIRE(frames.size() == 1);
  CHECK(frames[0].type == dentra::tion_lt::FRAME_TYPE_STATE_SET);
  const auto &req = frames[0].as<tionlt_state_set_req_t>();
  CHECK(req.data.fan_speed == 4);
  CHECK(req.data.button_presets.tmp[0] == 10);
  CHECK(req.data.button_presets.tmp[1] == 15);
  CHECK(req.data.button_presets.tmp[2] == 20);
  CHECK(req.data.button_presets.fan[0] == 1);
  CHECK(req.data.button_presets.fan[1] == 3);
  CHECK(req.data.button_presets.fan[2] == 5);
}

TEST_CASE("kiv: breezer in kiv mode is turned off with first speed on frost") {
  HarnessLtUart h;
  h.api.enable_kiv_support();
  receive_text(h, lt_state(10, false));
  {
    TionStateCall call(&h.api);
    call.set_fan_speed(0);
    call.perform();
  }
  REQUIRE(text(h.tx) == "set_speed 0\r\ngetstate\r\n");
  h.tx.clear();
  // breezer is in kiv mode, outdoor temperature drops
  std::string st = lt_state(3, false);
  st.replace(st.find("Speed: 2"), 8, "Speed: 0");
  receive_text(h, st);
  REQUIRE(h.api.get_state().fan_speed == 0);
  h.tx.clear();

  TionStateCall call(&h.api);
  call.set_target_temperature(20);
  call.perform();

  // turned off with first speed, so it can be turned on again
  CHECK(text(h.tx) == "set_speed 1\r\nset_temp 20\r\nstby\r\ngetstate\r\n");
}
