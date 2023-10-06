#include "esphome/components/climate/climate_mode.h"
#include "../components/tion-api/tion-api-4s-internal.h"
#include "../components/tion_4s/climate/tion_4s_climate.h"
#include "../components/tion_4s_uart/tion_4s_uart.h"

#include "test_api.h"
#include "test_vport.h"

DEFINE_TAG;

using namespace esphome;
using namespace esphome::tion;
using namespace dentra::tion;
using namespace dentra::tion_4s;

using Tion4sBleVPortApiTest = TionVPortApi<Tion4sBleIOTest::frame_spec_type, TionApi4s>;
// using Tion4sUartVPortApiTest = TionVPortApi<Tion4sUartIOTest::frame_spec_type, TionApi4s>;

class Tion4sUartVPortApiTest : public TionVPortApi<Tion4sUartIOTest::frame_spec_type, TionApi4s> {
 public:
  Tion4sUartVPortApiTest(Tion4sUartVPort *vport) : TionVPortApi<Tion4sUartIOTest::frame_spec_type, TionApi4s>(vport) {
    this->set_writer(
        TionApi4s::writer_type::create<Tion4sUartVPortApiTest, &Tion4sUartVPortApiTest::test_write_>(*this));

    this->state_.max_fan_speed = 6;
    this->state_.fan_speed = 1;
    this->state_.counters.work_time = 0xFFFF;
  }

 protected:
  tion4s_state_t state_{};
  bool test_write_(uint16_t type, const void *data, size_t size) {
    struct req_t {
      uint32_t request_id;
      tion4s_state_set_t data;
    } __attribute__((__packed__));

    if (type == FRAME_TYPE_STATE_SET && size == sizeof(req_t)) {
      auto req = static_cast<const req_t *>(data);

      this->state_.flags.power_state = req->data.power_state;
      this->state_.flags.heater_mode = req->data.heater_mode;
      if (req->data.fan_speed > 0 && req->data.fan_speed <= this->state_.max_fan_speed) {
        this->state_.fan_speed = req->data.fan_speed;
      }
      this->state_.target_temperature = req->data.target_temperature;
      this->state_.gate_position = req->data.gate_position;
      // add others

      this->on_state(this->state_, req->request_id);
    }
    return this->write_frame_(type, data, size);
  }
};

class Tion4sBleVPortTest : public Tion4sBleVPort {
 public:
  Tion4sBleVPortTest(Tion4sBleIOTest *io) : Tion4sBleVPort(io) {}
  // uint16_t get_state_type() const { return this->state_type_; }
};

class Tion4sTest : public Tion4sClimate {
 public:
  Tion4sTest(TionApi4s *api) : Tion4sClimate(api) { this->state_.counters.work_time = 0xFFFF; }
  // void enable_preset(climate::ClimatePreset preset) { this->enable_preset_(preset); }
  // void cancel_preset(climate::ClimatePreset preset) { this->cancel_preset_(preset); }
  tion4s_state_t &state() { return this->state_; };
};

/*
bool test_api_4s_x() {
  bool res = true;
  TestTionBleLtProtocol p;
  for (auto data : test_4s_data) {
    Api4sTest t4s(&p);
    // p.set_api(&t4s);
    for (auto d : data.frames) {
      p.read_data(cloak::from_hex(d));
    }
    test_check(res, t4s.received_struct_, data.await_struct);
  }
  return res;
}
*/

bool test_api_4s() {
  bool res = true;

  esphome::ble_client::BLEClient client;
  client.set_address(0x112233445566);
  client.connect();

  Tion4sBleIOTest io(&client);
  Tion4sBleVPortTest vport(&io);
  Tion4sBleVPortApiTest api(&vport);
  // Tion4sTest comp(&api);

  io.node_state = esphome::esp32_ble_tracker::ClientState::ESTABLISHED;
  // vport.set_persistent_connection(true);

  // cloak::setup_and_loop({&vport, &comp});

  api.request_state();
  res &= cloak::check_data("request_state", io, "80.0C.00.3A.AD 32.32 01.00.00.00 64.F7");

  api.request_dev_info();
  res &= cloak::check_data("request_dev_info", io, "80.0C.00.3A.AD 32.33 01.00.00.00 CE.A6");

  api.request_timer(0, 1);
  res &= cloak::check_data("request_timer", io, "80.11.00.3A.AD 32.34 01.00.00.00 01.00.00.00.00 DB.D5");

  api.request_timers(1);
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
  Tion4sTest comp(&api);

  cloak::setup_and_loop({&vport, &comp});

  auto call = comp.make_call();

  call.set_preset(esphome::climate::ClimatePreset::CLIMATE_PRESET_HOME);
  comp.control(call);

  call.set_preset(esphome::climate::ClimatePreset::CLIMATE_PRESET_NONE);
  comp.control(call);

  res &= cloak::check_data("target_temperature > 0", comp.target_temperature > 0, true);

  return res;
}

bool test_heat_cool() {
  bool res = true;

  esphome::uart::UARTComponent uart;
  Tion4sUartIOTest io(&uart);
  Tion4sUartVPort vport(&io);
  Tion4sUartVPortApiTest api(&vport);
  Tion4sTest comp(&api);

  cloak::setup_and_loop({&vport, &comp});

  for (auto mode : {climate::ClimateMode::CLIMATE_MODE_FAN_ONLY, climate::ClimateMode::CLIMATE_MODE_HEAT}) {
    auto call = comp.make_call();

    call.set_mode(mode);
    comp.control(call);

    call.set_mode(esphome::climate::ClimateMode::CLIMATE_MODE_OFF);
    comp.control(call);

    call.set_mode(esphome::climate::ClimateMode::CLIMATE_MODE_HEAT_COOL);
    comp.control(call);

    auto str = std::string("climate mode ") + LOG_STR_ARG(climate::climate_mode_to_string(mode));
    res &= cloak::check_data(str, comp.mode == mode, true);
  }

  return res;
}

REGISTER_TEST(test_api_4s);
REGISTER_TEST(test_presets);
REGISTER_TEST(test_heat_cool);
