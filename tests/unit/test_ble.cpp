#include "harness.h"

#include "tion-api-3s.h"
#include "tion-api-4s.h"
#include "tion-api-lt.h"
#include "tion-api-ble-3s.h"
#include "tion-api-ble-lt.h"

using dentra::tion::Tion3sApi;
using dentra::tion::Tion3sBleProtocol;
using dentra::tion::TionGatePosition;
using dentra::tion::TionStateCall;
using dentra::tion::TionLtApi;
using dentra::tion::TionLtBleProtocol;
using dentra::tion_4s::Tion4sApi;

namespace {

using Harness4sBle = test::BleHarness<Tion4sApi, TionLtBleProtocol>;
using HarnessLtBle = test::BleHarness<TionLtApi, TionLtBleProtocol>;
using Harness3sBle = test::BleHarness<Tion3sApi, Tion3sBleProtocol>;

// real 3s state: speed 1, gate outdoor, 23°C, heater+power+sound on, filter 79 days
const char *const STATE_3S = "B3.10 21.17.0B.00.00.00.00.4F.00.0E.2D.00.00.00.00.FF.FF 5A";

// real 4s state split into 3 ble packets: first (00), current (40), last (C0)
const char *const STATE_PACKETS[] = {
    "00.2F.00.3A.D1.31.32.01.00.00.00.00.00.00.00.3C.51.00.10.01",
    "40.0C.17.12.1E.71.EF.29.00.D8.16.1F.00.28.37.CE.00.FE.56.43",
    "C0.00.00.00.00.00.06.00.63.1A",
};

// real 4s device info split into 2 ble packets
const char *const DEV_INFO_PACKETS[] = {
    "00.25.00.3A.20.31.33.01.00.00.00.01.03.80.00.00.BC.02.01.00",
    "C0.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.05.23",
};

}  // namespace

TEST_CASE("4s ble: requests encoding") {
  Harness4sBle h;

  SUBCASE("first request_state also requests device info") {
    h.api.request_state();
    CHECK(h.take_tx() == test::hex("80.0C.00.3A.AD.32.33.01.00.00.00.CE.A6 80.0C.00.3A.AD.32.32.01.00.00.00.64.F7"));
  }
  SUBCASE("request_timer") {
    h.api.request_timer(0, 1);
    CHECK(h.take_tx() == test::hex("80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.00.DB.D5"));
  }
  SUBCASE("request_timers") {
    h.api.request_timers(1);
    CHECK(h.take_tx() == test::hex("80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.00.DB.D5"
                                   "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.01.CB.F4"
                                   "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.02.FB.97"
                                   "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.03.EB.B6"
                                   "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.04.9B.51"
                                   "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.05.8B.70"
                                   "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.06.BB.13"
                                   "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.07.AB.32"
                                   "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.08.5A.DD"
                                   "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.09.4A.FC"
                                   "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.0A.7A.9F"
                                   "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.0B.6A.BE"));
  }
}

TEST_CASE("4s ble: real multi-packet state response") {
  Harness4sBle h;
  h.receive(STATE_PACKETS[0]);
  h.receive(STATE_PACKETS[1]);
  // frame is passed to api only after the last packet
  CHECK(h.rx_frames == 0);
  h.receive(STATE_PACKETS[2]);
  REQUIRE(h.rx_frames == 1);

  const auto &st = h.api.get_state();
  CHECK_FALSE(st.power_state);
  // heater is present, but works in fan only mode
  CHECK_FALSE(st.heater_state);
  CHECK(st.led_state);
  CHECK_FALSE(st.sound_state);
  CHECK(st.gate_position == TionGatePosition::OUTDOOR);
  CHECK(st.fan_speed == 1);
  CHECK(st.target_temperature == 16);
  CHECK(st.outdoor_temperature == 12);
  CHECK(st.current_temperature == 23);
  CHECK(st.pcb_ctl_temperature == 18);
  CHECK(st.pcb_pwr_temperature == 30);
  CHECK(st.filter_time_left == 13514536);
  CHECK(st.work_time == 2748273);
  CHECK(st.fan_time == 2037464);
  CHECK(st.errors == 0);
}

TEST_CASE("4s ble: real multi-packet device info response") {
  Harness4sBle h;
  for (const auto *p : DEV_INFO_PACKETS) {
    h.receive(p);
  }
  REQUIRE(h.rx_frames == 1);
  CHECK(h.api.get_state().firmware_version != 0);
}

TEST_CASE("4s ble: packet out of order resets frame") {
  Harness4sBle h;
  // current packet without first one
  h.receive(STATE_PACKETS[1]);
  h.receive(STATE_PACKETS[2]);
  CHECK(h.rx_frames == 0);
  // full sequence after garbage is still parsed
  for (const auto *p : STATE_PACKETS) {
    h.receive(p);
  }
  CHECK(h.rx_frames == 1);
}

TEST_CASE("lt ble: requests encoding") {
  HarnessLtBle h;
  h.api.request_state();
  CHECK(h.take_tx() == test::hex("80.0C.00.3A.AD.09.40.01.00.00.00.D1.DC 80.0C.00.3A.AD.32.12.01.00.00.00.6C.43"));
}

TEST_CASE("3s ble: request_state") {
  Harness3sBle h;
  h.api.request_state();
  CHECK(h.take_tx() == test::hex("3D.01 00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00 5A"));
}

TEST_CASE("3s ble: real state packet") {
  Harness3sBle h;
  CHECK(h.receive(STATE_3S));
  REQUIRE(h.rx_frames == 1);
  const auto &st = h.api.get_state();
  CHECK(st.fan_speed == 1);
  CHECK(st.gate_position == TionGatePosition::OUTDOOR);
  CHECK(st.target_temperature == 23);
  CHECK(st.heater_state);
  CHECK(st.power_state);
  CHECK(st.sound_state);
}

TEST_CASE("3s ble: invalid packets are rejected") {
  Harness3sBle h;
  SUBCASE("short") { CHECK_FALSE(h.receive("B3.10 21.17.0B")); }
  SUBCASE("long") { CHECK_FALSE(h.receive(std::string(STATE_3S) + " 00")); }
  SUBCASE("bad magic") { CHECK_FALSE(h.receive("B3.10 21.17.0B.00.00.00.00.4F.00.0E.2D.00.00.00.00.FF.FF 5B")); }
  CHECK(h.rx_frames == 0);
}

TEST_CASE("3s ble: state call writes state set packet") {
  Harness3sBle h;
  h.receive(STATE_3S);
  h.tx.clear();

  TionStateCall call(&h.api);
  call.set_fan_speed(3);
  call.set_heater_state(false);
  call.perform();

  CHECK(h.take_tx() == test::hex("3D.02 03.17.02.0A.01.00.00.00.00.00.00.00.00.00.00.00.00 5A"));
}

TEST_CASE("3s ble: reset filter") {
  Harness3sBle h;
  h.receive(STATE_3S);
  h.tx.clear();

  h.api.reset_filter();

  CHECK(h.take_tx() == test::hex("3D.02 01.17.02.0B.01.02.00.00.00.00.00.00.00.00.00.00.00 5A"));
}

TEST_CASE("3s ble: oversized frame data is not sent") {
  Harness3sBle h;
  const uint8_t data[18]{1, 2, 3};
  CHECK_FALSE(h.protocol.write_frame(0x0102, data, sizeof(data)));
  CHECK(h.tx.empty());
}

TEST_CASE("3s ble: max size frame data is sent") {
  Harness3sBle h;
  uint8_t data[17]{};
  data[0] = 1;
  data[16] = 0x11;
  CHECK(h.protocol.write_frame(0x0102, data, sizeof(data)));
  CHECK(h.take_tx() == test::hex("02.01 01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.11 5A"));
}

namespace {

using dentra::tion_4s::tion4s_raw_frame_t;
using dentra::tion_4s::tion4s_state_t;
using dentra::tion_4s::tion4s_turbo_t;

struct Breezer4sBle {
  Harness4sBle h;
  tion4s_state_t st{};

  Breezer4sBle() {
    h.api.enable_native_boost_support();
    st.power_state = true;
    st.fan_speed = 2;
    st.max_fan_speed = 6;
    st.counters.work_time = 1000;
    h.receive(test::encode_ble<TionLtBleProtocol>(dentra::tion_4s::FRAME_TYPE_STATE_RSP,
                                                  tion4s_raw_frame_t<tion4s_state_t>{1, st}));
    h.tx.clear();
  }

  void turbo(bool active, uint16_t time) {
    h.receive(test::encode_ble<TionLtBleProtocol>(dentra::tion_4s::FRAME_TYPE_TURBO_RSP,
                                                  tion4s_raw_frame_t<tion4s_turbo_t>{1, {active, time, 0}}));
  }
};

}  // namespace

TEST_CASE("4s ble: native turbo state is received") {
  Breezer4sBle b;
  REQUIRE(b.h.rx_frames == 1);
  b.turbo(true, 600);
  REQUIRE(b.h.rx_frames == 2);
  CHECK(b.h.api.get_boost_time_left() == 600);
  b.turbo(false, 0);
  CHECK(b.h.api.get_boost_time_left() == 0);
}

TEST_CASE("4s ble: native turbo is enabled with turbo command") {
  Breezer4sBle b;
  b.h.api.set_boost_time(600);
  TionStateCall call(&b.h.api);
  static_cast<dentra::tion::TionApiBase &>(b.h.api).enable_boost(true, &call);
  call.perform();
  //                              head          type  ble id      req id      time  err crc
  CHECK(b.h.take_tx() == test::hex("80.13.00.3A.AD 30.41 01.00.00.00 01.00.00.00 58.02 00 E2.1B"));
}

TEST_CASE("4s ble: native turbo is reported as running boost") {
  Breezer4sBle b;
  b.turbo(true, 600);
  CHECK(b.h.api.is_boost_running());
  b.turbo(false, 0);
  CHECK_FALSE(b.h.api.is_boost_running());
}

TEST_CASE("4s ble: native turbo is disabled with zero turbo time") {
  Breezer4sBle b;
  b.turbo(true, 600);
  TionStateCall call(&b.h.api);
  static_cast<dentra::tion::TionApiBase &>(b.h.api).enable_boost(false, &call);
  call.perform();
  CHECK(b.h.take_tx() == test::hex("80.13.00.3A.AD 30.41 01.00.00.00 01.00.00.00 00.00 00 73.16"));
}

TEST_CASE("4s ble: auto mode does not interfere with native turbo") {
  Breezer4sBle b;
  b.h.api.set_auto_setpoint(700);
  b.h.api.set_auto_min_fan_speed(1);
  b.h.api.set_auto_max_fan_speed(4);
  b.st.ma_connected = true;
  b.h.receive(test::encode_ble<TionLtBleProtocol>(dentra::tion_4s::FRAME_TYPE_STATE_RSP,
                                                  tion4s_raw_frame_t<tion4s_state_t>{1, b.st}));
  REQUIRE(b.h.api.get_state().auto_state);
  b.turbo(true, 600);
  b.h.tx.clear();

  TionStateCall call(&b.h.api);
  CHECK_FALSE(b.h.api.auto_update(1400, &call));
  call.perform();
  CHECK(b.h.tx.empty());
}

TEST_CASE("4s ble: boost preset follows native turbo") {
  Breezer4sBle b;
  // native turbo ignores preset temperature and heater, state never matches the preset
  b.h.api.add_preset("boost", dentra::tion::TionApiBase::PresetData{.target_temperature = 20,
                                                                    .heater_state = 1,
                                                                    .power_state = 1,
                                                                    .fan_speed = -1,
                                                                    .gate_position = TionGatePosition::NONE,
                                                                    .auto_state = -1,
                                                                    .work_time = 0});
  b.h.api.set_boost_time(600);
  TionStateCall call(&b.h.api);
  b.h.api.enable_preset("boost", &call);
  call.perform();
  CHECK(strcasecmp(b.h.api.get_active_preset_name(), "none") == 0);

  b.turbo(true, 600);
  CHECK(strcasecmp(b.h.api.get_active_preset_name(), "boost") == 0);
  b.turbo(false, 0);
  CHECK(strcasecmp(b.h.api.get_active_preset_name(), "none") == 0);

  // later manual max speed does not activate boost preset
  b.st.fan_speed = 6;
  b.st.heater_state = true;
  b.st.target_temperature = 20;
  b.h.receive(test::encode_ble<TionLtBleProtocol>(dentra::tion_4s::FRAME_TYPE_STATE_RSP,
                                                  tion4s_raw_frame_t<tion4s_state_t>{1, b.st}));
  CHECK(strcasecmp(b.h.api.get_active_preset_name(), "none") == 0);
}

TEST_CASE("4s ble: native turbo state does not trigger soft boost logic") {
  Breezer4sBle b;
  b.turbo(true, 600);
  // state with non-max speed during native turbo must not be treated as boost cancel
  b.h.receive(test::encode_ble<TionLtBleProtocol>(dentra::tion_4s::FRAME_TYPE_STATE_RSP,
                                                  tion4s_raw_frame_t<tion4s_state_t>{1, b.st}));
  CHECK(b.h.tx.empty());
  CHECK(b.h.api.is_boost_running());
}
