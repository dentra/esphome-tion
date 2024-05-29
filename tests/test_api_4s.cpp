#include "esphome/components/climate/climate_mode.h"

#include "../components/tion-api/tion-api-4s-internal.h"
#include "../components/tion_4s_uart/tion_4s_uart_vport.h"
#include "../components/tion/climate/tion_climate.h"
#include "../components/tion/tion_component.h"

#include "test_api.h"
#include "test_vport.h"

DEFINE_TAG;

using namespace esphome;

using dentra::tion_4s::tion4s_state_set_req_t;
using dentra::tion_4s::tion4s_state_t;
using dentra::tion::TionGatePosition;
using dentra::tion::TionTraits;
using dentra::tion::TionState;
using dentra::tion_4s::Tion4sApi;
using esphome::tion::Tion4sApiComponent;

using Tion4sUartVPort = esphome::tion::Tion4sUartVPort;
using Tion4sBleVPort = esphome::tion::Tion4sBleVPort;

using Tion4sUartVPortApiTestBase = esphome::tion::TionVPortApi<Tion4sUartIOTest::frame_spec_type, Tion4sApi>;
// using Tion4sBleVPortApiTest = esphome::tion::TionVPortApi<Tion4sBleIOTest::frame_spec_type, Tion4sApi>;
class Tion4sBleVPortApiTest : public esphome::tion::TionVPortApi<Tion4sBleIOTest::frame_spec_type, Tion4sApi> {
 public:
  Tion4sBleVPortApiTest(vport_t *vport)
      : esphome::tion::TionVPortApi<Tion4sBleIOTest::frame_spec_type, Tion4sApi>(vport) {
    this->state_.firmware_version = 0xFFFF;
    this->state_.fan_speed = 1;
    this->state_.work_time = 0xFFFF;
  }
  bool request_dev_info() const { return this->request_dev_info_(); }
};

class Tion4sUartVPortApiTest : public Tion4sUartVPortApiTestBase {
 public:
  Tion4sUartVPortApiTest(Tion4sUartVPort *vport) : Tion4sUartVPortApiTestBase(vport) {
    this->set_writer(
        Tion4sApi::writer_type::create<Tion4sUartVPortApiTest, &Tion4sUartVPortApiTest::test_write_>(*this));
    // this->traits_.max_fan_speed = 6;
    this->state_.firmware_version = 0xFFFF;
    this->state_.fan_speed = 1;
    this->state_.work_time = 0xFFFF;
  }

  // TionState state_{};

 protected:
  bool test_write_(uint16_t type, const void *data, size_t size) {
    if (type == dentra::tion_4s::FRAME_TYPE_STATE_SET && size == sizeof(tion4s_state_set_req_t)) {
      const auto *req = static_cast<const tion4s_state_set_req_t *>(data);

      this->state_.power_state = req->data.power_state;
      this->state_.heater_state = req->data.heater_mode == tion4s_state_t::HEATER_MODE_HEATING;
      if (req->data.fan_speed > 0 && req->data.fan_speed <= this->traits_.max_fan_speed) {
        this->state_.fan_speed = req->data.fan_speed;
      }
      this->state_.target_temperature = req->data.target_temperature;
      this->state_.gate_position =                                          //-//
          req->data.gate_position == tion4s_state_t::GATE_POSITION_OUTDOOR  //-//
              ? TionGatePosition::OUTDOOR
              : TionGatePosition::INDOOR;
      this->state_.led_state = req->data.led_state;
      this->state_.sound_state = req->data.sound_state;
      // add others

      this->on_state_fn(this->state_, req->request_id);
    }
    return this->write_frame_(type, data, size);
  }
};

class Tion4sBleVPortTest : public Tion4sBleVPort {
 public:
  Tion4sBleVPortTest(Tion4sBleIOTest *io) : Tion4sBleVPort(io) {}
  // uint16_t get_state_type() const { return this->state_type_; }
};

class Tion4sTest : public tion::TionClimate {
 public:
  Tion4sTest(Tion4sApiComponent *api) : tion::TionClimate(api) {}

  void update_preset_service(std::string preset_str, std::string mode_str, int fan_speed, int target_temperature,
                             std::string gate_position_str) {
    auto preset = this->get_parent()->api()->get_preset(preset_str);
    preset.power_state = mode_str == "off" ? 0 : 1;
    preset.heater_state = mode_str == "heat" ? 1 : 0;
    preset.fan_speed = fan_speed;
    preset.target_temperature = target_temperature;
    preset.gate_position = gate_position_str == "outdoor"  ? TionGatePosition::OUTDOOR
                           : gate_position_str == "indoor" ? TionGatePosition::INDOOR
                           : gate_position_str == "mixed"  ? TionGatePosition::MIXED
                                                           : TionGatePosition::UNKNOWN;
    this->get_parent()->api()->add_preset(preset_str, preset);
  }

  uint8_t get_fan_speed() const {
    return this->custom_fan_mode.has_value() ? this->custom_fan_mode.value()[0] - '0' : 0;
  }

  Tion4sApi::PresetData get_preset(esphome::climate::ClimatePreset index) {
    return this->get_parent()->api()->get_preset(this->preset_index_map_[index]);
  }

 protected:
  const char *preset_index_map_[esphome::climate::CLIMATE_PRESET_ACTIVITY + 1] = {
      "none", "home", "away", "boost", "comfort", "eco", "sleep", "activity"};
};

bool test_api_4s() {
  bool res = true;

  esphome::ble_client::BLEClient client;
  client.set_address(0x112233445566);
  client.connect();

  Tion4sBleIOTest io(&client);
  Tion4sBleVPortTest vport(&io);
  Tion4sBleVPortApiTest api(&vport);

  io.node_state = esphome::esp32_ble_tracker::ClientState::ESTABLISHED;
  // vport.set_persistent_connection(true);

  cloak::setup_and_loop({&vport});

  api.request_state();
  vport.call_loop();
  res &= cloak::check_data("request_state", io, "80.0C.00.3A.AD 32.32 01.00.00.00 64.F7");

  api.request_dev_info();
  vport.call_loop();
  res &= cloak::check_data("request_dev_info", io, "80.0C.00.3A.AD 32.33 01.00.00.00 CE.A6");

  api.request_timer(0, 1);
  vport.call_loop();
  res &= cloak::check_data("request_timer", io, "80.11.00.3A.AD 32.34 01.00.00.00 01.00.00.00.00 DB.D5");

  api.request_timers(1);
  for (int i = 0; i < 12; i++) {
    vport.call_loop();
  }
  res &= cloak::check_data("request_timers", io,
                           "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.00.DB.D5"
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
                           "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.0B.6A.BE");

  return res;
}

bool test_presets() {
  bool res = true;

  esphome::uart::UARTComponent uart;
  Tion4sUartIOTest io(&uart);
  Tion4sUartVPort vport(&io);
  Tion4sUartVPortApiTest api(&vport);
  Tion4sApiComponent capi(&api, vport.get_type());
  Tion4sTest comp(&capi);

  capi.add_preset("home", {
                              .target_temperature = 23,
                              .heater_state = 1,
                              .power_state = 1,
                              .fan_speed = 2,
                              .gate_position = TionGatePosition::NONE,
                          });
  capi.add_preset("boost", {
                               .target_temperature = 16,
                               .heater_state = 0,
                               .power_state = 1,
                               .fan_speed = 6,
                               .gate_position = TionGatePosition::NONE,
                           });

  cloak::setup_and_loop({&vport, &comp});

  auto call = comp.make_call();

  call.set_preset(esphome::climate::ClimatePreset::CLIMATE_PRESET_HOME);
  comp.control(call);

  call.set_preset(esphome::climate::ClimatePreset::CLIMATE_PRESET_NONE);
  comp.control(call);

  // проверяем, что не упали в неинициализированное состояние
  res &= cloak::check_data("target_temperature > 0", comp.target_temperature > 0, true);

  comp.control(comp.make_call().set_fan_mode(std::string("3")));

  call.set_preset(esphome::climate::ClimatePreset::CLIMATE_PRESET_BOOST);
  comp.control(call);

  res &= cloak::check_data("fan_speed == 6", comp.get_fan_speed(), 6);

  call.set_preset(esphome::climate::ClimatePreset::CLIMATE_PRESET_NONE);
  comp.control(call);

  // сейчас переключение на none не приводит к восстановлению состояния
  // res &= cloak::check_data("fan_speed == 3", comp.get_fan_speed(), 3);
  res &= cloak::check_data("fan_speed == 6", comp.get_fan_speed(), 6);

  return res;
}

bool test_heat_cool() {
  bool res = true;

  esphome::uart::UARTComponent uart;
  Tion4sUartIOTest io(&uart);
  Tion4sUartVPort vport(&io);
  Tion4sUartVPortApiTest api(&vport);
  Tion4sApiComponent capi(&api, vport.get_type());
  Tion4sTest comp(&capi);

  cloak::setup_and_loop({&vport, &comp});

  for (auto mode : {climate::ClimateMode::CLIMATE_MODE_FAN_ONLY, climate::ClimateMode::CLIMATE_MODE_HEAT}) {
    auto call = comp.make_call();

    call.set_mode(mode);
    comp.control(call);

    call.set_mode(esphome::climate::ClimateMode::CLIMATE_MODE_OFF);
    comp.control(call);

    call.set_mode(esphome::climate::ClimateMode::CLIMATE_MODE_HEAT_COOL);
    comp.control(call);

    comp.call_loop();
    vport.call_loop();

    auto str = std::string("climate mode ") + LOG_STR_ARG(climate::climate_mode_to_string(mode));
    res &= cloak::check_data(str, comp.mode == mode, true);
  }

  return res;
}

bool test_preset_update() {
  bool res = true;

  esphome::uart::UARTComponent uart;
  Tion4sUartIOTest io(&uart);
  Tion4sUartVPort vport(&io);
  Tion4sUartVPortApiTest api(&vport);
  Tion4sApiComponent capi(&api, vport.get_type());
  Tion4sTest comp(&capi);

  cloak::setup_and_loop({&vport, &comp});

  comp.update_preset_service("eco", "fan_only", 1, 11, "outdoor");
  auto preset = comp.get_preset(esphome::climate::CLIMATE_PRESET_ECO);
  res &= cloak::check_data("preset.fan_speed==1", preset.fan_speed, 1);
  res &= cloak::check_data("preset.target_temperature==11", preset.target_temperature, 11);
  res &= cloak::check_data("preset.mode==fan_only", preset.heater_state, 0);

  res &= cloak::check_data("preset.gate_position==outdoor", (uint8_t) preset.gate_position,
                           (uint8_t) TionGatePosition::OUTDOOR);

  comp.update_preset_service("eco", "fan_only", 1, 11, "indoor");
  preset = comp.get_preset(esphome::climate::CLIMATE_PRESET_ECO);
  res &= cloak::check_data("preset.gate_position==indoor", (uint8_t) preset.gate_position,
                           (uint8_t) TionGatePosition::INDOOR);

  comp.update_preset_service("eco", "fan_only", 1, 11, "mixed");
  preset = comp.get_preset(esphome::climate::CLIMATE_PRESET_ECO);
  res &= cloak::check_data("preset.gate_position==mixed", (uint8_t) preset.gate_position,
                           (uint8_t) TionGatePosition::MIXED);

  comp.update_preset_service("eco", "off", 1, 11, "mixed");
  preset = comp.get_preset(esphome::climate::CLIMATE_PRESET_ECO);
  res &= cloak::check_data("preset.mode==off", preset.power_state, 0);

  return res;
}

bool test_batch() {
  bool res = true;

  esphome::uart::UARTComponent uart;
  Tion4sUartIOTest io(&uart);
  Tion4sUartVPort vport(&io);
  Tion4sUartVPortApiTest api(&vport);
  Tion4sApiComponent capi(&api, vport.get_type());
  Tion4sTest comp(&capi);

  capi.set_batch_timeout(3000);
  cloak::setup_and_loop({&vport, &comp});

  // comp.test_timeout(true);
  capi.test_timeout(true);

  auto *call = capi.make_call();
  call->set_led_state(true);
  call->perform();
  vport.call_loop();

  call->set_sound_state(true);
  call->perform();
  vport.call_loop();

  call->set_heater_state(true);
  call->set_fan_speed(3);
  call->set_target_temperature(23);
  call->perform();
  vport.call_loop();

  capi.test_timeout(false);
  capi.call_loop();
  vport.call_loop();

  res &= cloak::check_data("batch data led_state", api.get_state().led_state, true);
  res &= cloak::check_data("batch data sound_state", api.get_state().sound_state, true);
  res &= cloak::check_data("batch data heater_state", api.get_state().heater_state, true);
  res &= cloak::check_data("batch data fan_speed", api.get_state().fan_speed, 3);
  res &= cloak::check_data("batch data target_temperature", api.get_state().target_temperature, 23);

  return res;
}

template<typename mode_type, mode_type off_value> struct TionPresetDataTest {
  uint8_t fan_speed;
  int8_t target_temperature;
  mode_type mode;
  TionGatePosition gate_position;
  bool is_initialized() const { return this->fan_speed != 0; }
  bool is_enabled() const { return this->mode != off_value; }
  // uint8_t misalign[10];
} PACKED;

using ClimatePresetDataTest1 = TionPresetDataTest<esphome::climate::ClimateMode, esphome::climate::CLIMATE_MODE_OFF>;
using FanPresetDataTest1 = TionPresetDataTest<bool, false>;

struct ClimatePresetDataTest2 : public ClimatePresetDataTest1 {};

struct FanPresetDataTest2 : public TionPresetDataTest<bool, false> {
} __attribute__((aligned(sizeof(uint32_t))));

bool test_ttt() {
  ClimatePresetDataTest1 cdata1{.mode = esphome::climate::CLIMATE_MODE_HEAT};
  printf("size=%zu, speed=%u, mode=%u, is_enabled=%s\n", sizeof(cdata1), cdata1.fan_speed, cdata1.mode,
         ONOFF(cdata1.is_enabled()));

  ClimatePresetDataTest2 cdata2{};
  printf("size=%zu, speed=%u, mode=%u, is_enabled=%s\n", sizeof(cdata2), cdata2.fan_speed, cdata2.mode,
         ONOFF(cdata2.is_enabled()));

  FanPresetDataTest1 fdata1{.mode = true};
  printf("size=%zu, speed=%u, mode=%u, is_enabled=%s\n", sizeof(fdata1), fdata1.fan_speed, fdata1.mode,
         ONOFF(fdata1.is_enabled()));

  FanPresetDataTest2 fdata2{};
  printf("size=%zu, speed=%u, mode=%u, is_enabled=%s\n", sizeof(fdata2), fdata2.fan_speed, fdata2.mode,
         ONOFF(fdata2.is_enabled()));

  return true;
}
REGISTER_TEST(test_ttt);

REGISTER_TEST(test_api_4s);
REGISTER_TEST(test_presets);
REGISTER_TEST(test_heat_cool);
REGISTER_TEST(test_preset_update);
REGISTER_TEST(test_batch);
