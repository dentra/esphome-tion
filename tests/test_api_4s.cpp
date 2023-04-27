#include "../components/tion-api/tion-api-4s.h"
#include "../components/tion_4s/tion_4s.h"
#include "../components/tion_4s_uart/tion_4s_uart.h"

#include "test_api.h"
#include "test_vport.h"

DEFINE_TAG;

using namespace dentra::tion;

using Tion4sBleVPortApiTest = esphome::tion::TionVPortApi<Tion4sBleIOTest::frame_spec_type, dentra::tion::TionApi4s>;
using Tion4sUartVPortApiTest = esphome::tion::TionVPortApi<Tion4sUartIOTest::frame_spec_type, dentra::tion::TionApi4s>;

class Tion4sBleVPortTest : public esphome::tion::Tion4sBleVPort {
 public:
  Tion4sBleVPortTest(Tion4sBleIOTest *io) : esphome::tion::Tion4sBleVPort(io) {}
  uint16_t get_state_type() const { return this->state_type_; }
};

class Tion4sTest : public esphome::tion::Tion4s {
 public:
  Tion4sTest(dentra::tion::TionApi4s *api) : esphome::tion::Tion4s(api) {}
  // void enable_preset(climate::ClimatePreset preset) { this->enable_preset_(preset); }
  // void cancel_preset(climate::ClimatePreset preset) { this->cancel_preset_(preset); }
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

  api.request_dev_status();
  res &= cloak::check_data("request_dev_status", io, "80.0C.00.3A.AD 32.33 01.00.00.00 CE.A6");

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
  esphome::tion::Tion4sUartVPort vport(&io);
  Tion4sUartVPortApiTest api(&vport);
  Tion4sTest comp(&api);

  cloak::setup_and_loop({&vport, &comp});

  auto call = comp.make_call();

  call.set_preset(esphome::climate::ClimatePreset::CLIMATE_PRESET_HOME);
  comp.control(call);
  comp.publish_state();

  call.set_preset(esphome::climate::ClimatePreset::CLIMATE_PRESET_NONE);
  comp.control(call);
  comp.publish_state();

  cloak::check_data("target_temperature > 0", comp.target_temperature > 0, true);

  return res;
}

REGISTER_TEST(test_api_4s);
REGISTER_TEST(test_presets);
