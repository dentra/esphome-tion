// Common TionApiBase logic: presets, boost, auto mode. Uses 4S over UART as a carrier.
#include <algorithm>
#include <cstdarg>

#include "harness.h"

#include "tion-api-4s.h"
#include "tion-api-uart-4s.h"

using dentra::tion::Tion4sUartProtocol;
using dentra::tion::TionApiBase;
using dentra::tion::TionGatePosition;
using dentra::tion::TionStateCall;
using dentra::tion_4s::tion4s_raw_frame_t;
using dentra::tion_4s::tion4s_state_set_t;
using dentra::tion_4s::tion4s_state_t;
using dentra::tion_4s::Tion4sApi;

using namespace dentra::tion_4s;

namespace {

/// Simulates 4S breezer: answers polls and applies state set requests.
class Breezer4s {
 public:
  test::UartHarness<Tion4sApi, Tion4sUartProtocol> h;
  tion4s_state_t st{};

  Breezer4s() {
    test::FakeClock::set(1000);
    st.power_state = true;
    st.heater_mode = tion4s_state_t::HEATER_MODE_FANONLY;
    st.gate_position = tion4s_state_t::GATE_POSITION_OUTDOOR;
    st.target_temperature = 16;
    st.fan_speed = 2;
    st.max_fan_speed = 6;
    st.counters.work_time = 1000;
    this->respond(1);
  }

  Tion4sApi &api() { return this->h.api; }
  // enable_boost(bool) is hidden in Tion4sApi
  TionApiBase &base() { return this->h.api; }

  /// Sends current breezer state as a response with request_id.
  void respond(uint32_t request_id) {
    this->h.receive(test::encode_uart<Tion4sUartProtocol>(FRAME_TYPE_STATE_RSP,
                                                           tion4s_raw_frame_t<tion4s_state_t>{request_id, this->st}));
  }

  /// Takes pending state set request from api, does not apply it.
  test::Frame take_set() {
    const auto frames = test::decode_uart<Tion4sUartProtocol>(this->h.tx);
    this->h.tx.clear();
    for (auto &&f : frames) {
      if (f.type == FRAME_TYPE_STATE_SET) {
        return f;
      }
    }
    FAIL("no state set request");
    return {};
  }

  /// Applies state set request and answers it.
  void apply(const tion4s_raw_frame_t<tion4s_state_set_t> &req) {
    this->st.power_state = req.data.power_state;
    this->st.heater_mode = req.data.heater_mode;
    this->st.gate_position = req.data.gate_position;
    this->st.target_temperature = req.data.target_temperature;
    this->st.fan_speed = req.data.fan_speed;
    // 4s reports auto state as ma_connected
    this->st.ma_connected = req.data.ma_connected;
    this->respond(req.request_id);
  }

  /// Applies all pending requests.
  void process() {
    while (!this->h.tx.empty()) {
      const auto frames = test::decode_uart<Tion4sUartProtocol>(this->h.tx);
      this->h.tx.clear();
      for (auto &&f : frames) {
        if (f.type == FRAME_TYPE_STATE_SET) {
          this->apply(f.as<tion4s_raw_frame_t<tion4s_state_set_t>>());
        } else if (f.type == FRAME_TYPE_STATE_REQ) {
          this->respond(1);
        }
      }
    }
  }

  void work(uint32_t seconds) { this->st.counters.work_time += seconds; }

  void call(std::function<void(TionStateCall &)> &&fn) {
    TionStateCall call(&this->api());
    fn(call);
    call.perform();
  }
};

}  // namespace

TEST_CASE("presets: activate and modify") {
  Breezer4s b;
  b.api().add_preset("home", TionApiBase::PresetData{.target_temperature = 20,
                                                     .heater_state = 1,
                                                     .power_state = 1,
                                                     .fan_speed = 3,
                                                     .gate_position = TionGatePosition::NONE});

  b.call([&](auto &c) { b.api().enable_preset("home", &c); });
  b.process();

  CHECK(b.st.fan_speed == 3);
  CHECK(b.st.target_temperature == 20);
  CHECK(static_cast<int>(b.st.heater_mode) == tion4s_state_t::HEATER_MODE_HEATING);
  CHECK(strcasecmp(b.api().get_active_preset_name(), "home") == 0);

  // manual change resets active preset
  b.call([](auto &c) { c.set_fan_speed(4); });
  b.process();
  CHECK(strcasecmp(b.api().get_active_preset_name(), "none") == 0);
}

TEST_CASE("boost: runs for configured time and restores state") {
  Breezer4s b;
  b.api().set_boost_time(60);

  b.call([&](auto &c) { b.base().enable_boost(true, &c); });
  b.process();
  CHECK(b.api().is_boost_running());
  CHECK(b.st.fan_speed == 6);

  b.work(30);
  b.respond(1);
  CHECK(b.api().is_boost_running());
  CHECK(b.api().get_boost_time_left() == 30);

  b.work(31);
  b.respond(1);
  b.process();
  CHECK_FALSE(b.api().is_boost_running());
  CHECK(b.st.fan_speed == 2);
}

TEST_CASE("boost: user speed change cancels boost") {
  Breezer4s b;
  b.api().set_boost_time(60);
  b.call([&](auto &c) { b.base().enable_boost(true, &c); });
  b.process();
  REQUIRE(b.api().is_boost_running());

  b.call([](auto &c) { c.set_fan_speed(3); });
  b.process();

  CHECK_FALSE(b.api().is_boost_running());
}

// bugs.txt: boost sometimes does not start, fan stays at 6 forever (manual) or auto max speed (auto).
// A poll response sent by the breezer before it applied boost arrives after boost was enabled.
TEST_CASE("boost: stale poll response does not cancel boost") {
  Breezer4s b;
  b.api().set_boost_time(60);

  b.call([&](auto &c) { b.base().enable_boost(true, &c); });
  const auto boost_frame = b.take_set();
  const auto &boost_req = boost_frame.as<tion4s_raw_frame_t<tion4s_state_set_t>>();
  REQUIRE(boost_req.data.fan_speed == 6);

  // poll response with old speed arrives before the boost request is applied
  b.respond(1);
  b.process();
  // breezer applies boost request
  b.apply(boost_req);
  b.process();

  CHECK(b.api().is_boost_running());
  CHECK(b.st.fan_speed == 6);
}

TEST_CASE("boost: stale poll response in auto mode does not cancel boost") {
  Breezer4s b;
  b.api().set_auto_setpoint(700);
  b.api().set_auto_min_fan_speed(1);
  b.api().set_auto_max_fan_speed(4);
  b.api().set_boost_time(60);
  b.call([](auto &c) { c.set_auto_state(true); });
  b.process();
  REQUIRE(b.api().get_state().auto_state);

  b.call([&](auto &c) { b.base().enable_boost(true, &c); });
  const auto boost_frame = b.take_set();
  const auto &boost_req = boost_frame.as<tion4s_raw_frame_t<tion4s_state_set_t>>();

  b.respond(1);
  b.process();
  b.apply(boost_req);
  b.process();
  // co2 is high, auto mode must not interfere with boost
  b.call([&](auto &c) { b.api().auto_update(1400, &c); });
  b.process();

  CHECK(b.api().is_boost_running());
  CHECK(b.st.fan_speed == 6);
}

TEST_CASE("boost: unconfirmed boost falls back to user action after confirm timeout") {
  Breezer4s b;
  b.api().set_boost_time(60);

  b.call([&](auto &c) { b.base().enable_boost(true, &c); });
  // breezer ignores boost request
  b.take_set();
  b.respond(1);
  CHECK(b.api().is_boost_running());

  test::FakeClock::advance(TION_BOOST_CONFIRM_TIMEOUT);
  b.respond(1);
  b.process();
  CHECK_FALSE(b.api().is_boost_running());
  CHECK(b.st.fan_speed == 2);
}

TEST_CASE("boost: power off by remote before confirmation is kept") {
  Breezer4s b;
  b.api().set_boost_time(60);

  b.call([&](auto &c) { b.base().enable_boost(true, &c); });
  b.take_set();
  // user turns breezer off with remote before boost was applied
  b.st.power_state = false;
  b.respond(1);
  CHECK(b.api().is_boost_running());

  test::FakeClock::advance(TION_BOOST_CONFIRM_TIMEOUT);
  b.respond(1);
  b.process();
  CHECK_FALSE(b.api().is_boost_running());
  CHECK_FALSE(static_cast<bool>(b.st.power_state));
}

TEST_CASE("boost: preset with custom fan speed") {
  Breezer4s b;
  b.api().add_preset("boost", TionApiBase::PresetData{.target_temperature = -1,
                                                      .heater_state = -1,
                                                      .power_state = 1,
                                                      .fan_speed = 4,
                                                      .gate_position = TionGatePosition::NONE,
                                                      .auto_state = -1,
                                                      .work_time = 0});
  b.api().set_boost_time(60);

  b.call([&](auto &c) { b.base().enable_boost(true, &c); });
  b.process();
  CHECK(b.st.fan_speed == 4);
  CHECK(b.api().is_boost_running());

  test::FakeClock::advance(TION_BOOST_CONFIRM_TIMEOUT);
  b.work(30);
  b.respond(1);
  b.process();
  CHECK(b.api().is_boost_running());
  CHECK(b.st.fan_speed == 4);

  b.work(31);
  b.respond(1);
  b.process();
  CHECK_FALSE(b.api().is_boost_running());
  CHECK(b.st.fan_speed == 2);
}

TEST_CASE("boost: canceled boost preset does not become active later") {
  Breezer4s b;
  b.api().add_preset("boost", TionApiBase::PresetData{.target_temperature = -1,
                                                      .heater_state = -1,
                                                      .power_state = 1,
                                                      .fan_speed = -1,
                                                      .gate_position = TionGatePosition::NONE,
                                                      .auto_state = -1,
                                                      .work_time = 0});

  b.call([&](auto &c) { b.api().enable_preset("boost", &c); });
  // breezer ignores boost request
  b.take_set();
  test::FakeClock::advance(TION_BOOST_CONFIRM_TIMEOUT);
  b.respond(1);
  b.process();
  REQUIRE_FALSE(b.api().is_boost_running());

  // user sets max speed manually
  b.call([](auto &c) { c.set_fan_speed(6); });
  b.process();
  CHECK(strcasecmp(b.api().get_active_preset_name(), "none") == 0);
}

TEST_CASE("presets: boost preset is defined by name") {
  Breezer4s b;
  b.api().add_preset("Boost", TionApiBase::PresetData{.target_temperature = -1,
                                                      .heater_state = -1,
                                                      .power_state = 1,
                                                      .fan_speed = -1,
                                                      .gate_position = TionGatePosition::NONE,
                                                      .auto_state = -1,
                                                      .work_time = 0});
  b.api().set_boost_time(60);

  // climate passes standard preset name in upper case
  b.call([&](auto &c) { b.api().enable_preset("BOOST", &c); });
  b.process();
  CHECK(b.api().is_boost_running());
  CHECK(b.st.fan_speed == 6);

  b.call([&](auto &c) { b.api().enable_preset("none", &c); });
  b.process();
  CHECK_FALSE(b.api().is_boost_running());
  CHECK(b.st.fan_speed == 2);
}

TEST_CASE("presets: deprecated work_time still makes boost preset") {
  Breezer4s b;
  b.api().add_preset("turbo", TionApiBase::PresetData{.target_temperature = -1,
                                                      .heater_state = -1,
                                                      .power_state = 1,
                                                      .fan_speed = 5,
                                                      .gate_position = TionGatePosition::NONE,
                                                      .auto_state = -1,
                                                      .work_time = 600});

  b.call([&](auto &c) { b.api().enable_preset("turbo", &c); });
  b.process();
  CHECK(b.api().is_boost_running());
  CHECK(b.st.fan_speed == 5);
  // duration is not taken from work_time
  CHECK(b.api().get_boost_time() == TION_BOOST_TIME);
  CHECK(b.api().get_boost_time_left() == TION_BOOST_TIME);
}

TEST_CASE("presets: only boost preset makes presets available") {
  Breezer4s b;
  CHECK_FALSE(b.api().has_presets());
  b.api().add_preset("boost", TionApiBase::PresetData{.target_temperature = -1,
                                                      .heater_state = -1,
                                                      .power_state = 1,
                                                      .fan_speed = -1,
                                                      .gate_position = TionGatePosition::NONE,
                                                      .auto_state = -1,
                                                      .work_time = 0});
  CHECK(b.api().has_presets());
  const auto presets = b.api().get_presets();
  REQUIRE(presets.size() == 2);
  CHECK(strcasecmp(presets[1], "boost") == 0);
}

TEST_CASE("presets: second boost preset fully overrides first") {
  Breezer4s b;
  b.api().add_preset("turbo", TionApiBase::PresetData{.target_temperature = -1,
                                                      .heater_state = -1,
                                                      .power_state = 1,
                                                      .fan_speed = 4,
                                                      .gate_position = TionGatePosition::NONE,
                                                      .auto_state = -1,
                                                      .work_time = 600});
  b.api().add_preset("boost", TionApiBase::PresetData{.target_temperature = 20,
                                                      .heater_state = -1,
                                                      .power_state = 1,
                                                      .fan_speed = -1,
                                                      .gate_position = TionGatePosition::NONE,
                                                      .auto_state = -1,
                                                      .work_time = 0});

  b.call([&](auto &c) { b.api().enable_preset("boost", &c); });
  b.process();
  CHECK(b.api().is_boost_running());
  // fan speed is not inherited from "turbo"
  CHECK(b.st.fan_speed == 6);
  CHECK(b.st.target_temperature == 20);
}

TEST_CASE("presets: log shows only defined parameters") {
  std::vector<std::string> logs;
  dentra::tion::set_logger([&logs](int, const char *, int, const char *format, va_list args) {
    char buf[256];
    vsnprintf(buf, sizeof(buf), format, args);
    logs.emplace_back(buf);
  });
  Breezer4s b;
  logs.clear();
  b.api().add_preset("mixed", TionApiBase::PresetData{.target_temperature = -1,
                                                      .heater_state = -1,
                                                      .power_state = 1,
                                                      .fan_speed = -1,
                                                      .gate_position = TionGatePosition::MIXED,
                                                      .auto_state = 0,
                                                      .work_time = 0});
  b.api().add_preset("boost", TionApiBase::PresetData{.target_temperature = 20,
                                                      .heater_state = -1,
                                                      .power_state = 1,
                                                      .fan_speed = -1,
                                                      .gate_position = TionGatePosition::NONE,
                                                      .auto_state = -1,
                                                      .work_time = 0});
  b.api().add_preset("full", TionApiBase::PresetData{.target_temperature = 25,
                                                     .heater_state = 0,
                                                     .power_state = 0,
                                                     .fan_speed = 6,
                                                     .gate_position = TionGatePosition::OUTDOOR,
                                                     .auto_state = 0,
                                                     .work_time = 0});
  dentra::tion::set_logger(nullptr);
  CHECK(std::count(logs.begin(), logs.end(), "Setup preset 'mixed': power=on, gate=mixed, auto=off") == 1);
  CHECK(std::count(logs.begin(), logs.end(), "Update boost preset 'boost': temp=20") == 1);
  CHECK(std::count(logs.begin(), logs.end(),
                   "Setup preset 'full': power=off, heat=off, fan=6, temp=25, gate=outdoor, auto=off") == 1);
}

TEST_CASE("presets: boost log ignores power, gate and auto") {
  std::vector<std::string> logs;
  dentra::tion::set_logger([&logs](int, const char *, int, const char *format, va_list args) {
    char buf[256];
    vsnprintf(buf, sizeof(buf), format, args);
    logs.emplace_back(buf);
  });
  Breezer4s b;
  logs.clear();
  b.api().add_preset("boost", TionApiBase::PresetData{.target_temperature = 25,
                                                      .heater_state = 1,
                                                      .power_state = 1,
                                                      .fan_speed = 6,
                                                      .gate_position = TionGatePosition::OUTDOOR,
                                                      .auto_state = 0,
                                                      .work_time = 0});
  dentra::tion::set_logger(nullptr);
  CHECK(std::count(logs.begin(), logs.end(), "Update boost preset 'boost': heat=on, fan=6, temp=25") == 1);
}

TEST_CASE("presets: other presets are not boost") {
  Breezer4s b;
  b.api().add_preset("max", TionApiBase::PresetData{.target_temperature = -1,
                                                    .heater_state = -1,
                                                    .power_state = 1,
                                                    .fan_speed = 6,
                                                    .gate_position = TionGatePosition::NONE,
                                                    .auto_state = -1,
                                                    .work_time = 0});

  b.call([&](auto &c) { b.api().enable_preset("max", &c); });
  b.process();
  CHECK_FALSE(b.api().is_boost_running());
  CHECK(b.st.fan_speed == 6);
}

TEST_CASE("auto: settings change does not notify state before it is received") {
  test::UartHarness<Tion4sApi, Tion4sUartProtocol> h;
  int notified = 0;
  h.api.set_on_state([&](const dentra::tion::TionState &, uint32_t) { notified++; });

  h.api.set_auto_setpoint(700);
  h.api.set_auto_min_fan_speed(1);
  h.api.set_auto_max_fan_speed(4);
  CHECK(notified == 0);
}

TEST_CASE("auto: settings change notifies state after it is received") {
  Breezer4s b;
  int notified = 0;
  b.api().set_on_state([&](const dentra::tion::TionState &, uint32_t) { notified++; });

  b.api().set_auto_setpoint(700);
  CHECK(notified == 1);
}

TEST_CASE("presets: none cancels boost preset waiting for confirmation") {
  Breezer4s b;
  b.api().add_preset("boost", TionApiBase::PresetData{.target_temperature = -1,
                                                      .heater_state = -1,
                                                      .power_state = 1,
                                                      .fan_speed = -1,
                                                      .gate_position = TionGatePosition::NONE,
                                                      .auto_state = -1,
                                                      .work_time = 0});
  b.api().set_boost_time(60);

  b.call([&](auto &c) { b.api().enable_preset("boost", &c); });
  // breezer did not apply boost yet
  b.take_set();
  REQUIRE(b.api().is_boost_running());

  b.call([&](auto &c) { b.api().enable_preset("none", &c); });
  b.process();

  CHECK_FALSE(b.api().is_boost_running());
  CHECK(b.st.fan_speed == 2);
}

TEST_CASE("presets: regular preset cancels running boost") {
  Breezer4s b;
  b.api().set_boost_time(60);
  b.api().add_preset("eco", TionApiBase::PresetData{.target_temperature = 15,
                                                    .heater_state = 0,
                                                    .power_state = 1,
                                                    .fan_speed = -1,
                                                    .gate_position = TionGatePosition::NONE,
                                                    .auto_state = -1,
                                                    .work_time = 0});
  // heater on before boost
  b.call([](auto &c) {
    c.set_heater_state(true);
    c.set_target_temperature(20);
  });
  b.process();
  b.call([&](auto &c) { b.base().enable_boost(true, &c); });
  b.process();
  REQUIRE(b.api().is_boost_running());

  b.call([&](auto &c) { b.api().enable_preset("eco", &c); });
  b.process();
  CHECK_FALSE(b.api().is_boost_running());
  // speed is restored from before boost, preset params are applied
  CHECK(b.st.fan_speed == 2);
  CHECK(b.st.target_temperature == 15);
  CHECK(static_cast<int>(b.st.heater_mode) == tion4s_state_t::HEATER_MODE_FANONLY);

  // boost time is over, nothing is restored over the preset
  b.work(61);
  b.respond(1);
  b.process();
  CHECK(b.st.target_temperature == 15);
  CHECK(strcasecmp(b.api().get_active_preset_name(), "eco") == 0);
}

TEST_CASE("boost: preset with lower speed starts at max speed") {
  Breezer4s b;
  b.api().add_preset("boost", TionApiBase::PresetData{.target_temperature = -1,
                                                      .heater_state = -1,
                                                      .power_state = 1,
                                                      .fan_speed = 4,
                                                      .gate_position = TionGatePosition::NONE,
                                                      .auto_state = -1,
                                                      .work_time = 0});
  b.api().set_boost_time(60);
  b.call([](auto &c) { c.set_fan_speed(6); });
  b.process();

  b.call([&](auto &c) { b.base().enable_boost(true, &c); });
  b.process();

  CHECK(b.api().is_boost_running());
  CHECK(b.st.fan_speed == 4);
}

TEST_CASE("boost: not started when already at boost speed") {
  Breezer4s b;
  b.api().set_boost_time(60);
  b.call([](auto &c) { c.set_fan_speed(6); });
  b.process();

  b.call([&](auto &c) { b.base().enable_boost(true, &c); });
  b.process();

  CHECK_FALSE(b.api().is_boost_running());
}

TEST_CASE("auto: fan speed follows co2 within limits") {
  Breezer4s b;
  b.api().set_auto_setpoint(700);
  b.api().set_auto_min_fan_speed(1);
  b.api().set_auto_max_fan_speed(4);
  REQUIRE(b.api().auto_is_valid());

  b.call([](auto &c) { c.set_auto_state(true); });
  b.process();
  REQUIRE(b.api().get_state().auto_state);

  for (int i = 0; i < 20; i++) {
    b.call([&](auto &c) { b.api().auto_update(1400, &c); });
    b.process();
  }
  CHECK(b.st.fan_speed == 4);

  for (int i = 0; i < 50; i++) {
    b.call([&](auto &c) { b.api().auto_update(450, &c); });
    b.process();
  }
  CHECK(b.st.fan_speed == 1);
}

TEST_CASE("auto: boost has priority") {
  Breezer4s b;
  b.api().set_auto_setpoint(700);
  b.api().set_auto_min_fan_speed(1);
  b.api().set_auto_max_fan_speed(4);
  b.api().set_boost_time(60);
  b.call([](auto &c) { c.set_auto_state(true); });
  b.process();

  b.call([&](auto &c) { b.base().enable_boost(true, &c); });
  b.process();
  REQUIRE(b.api().is_boost_running());

  b.call([&](auto &c) { CHECK_FALSE(b.api().auto_update(1400, &c)); });
  b.process();
  CHECK(b.st.fan_speed == 6);
  // auto mode stays enabled during boost
  CHECK(b.api().get_state().auto_state);
}
