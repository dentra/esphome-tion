#include <cstring>
#include <vector>

#include "esphome/components/vport/vport_ble.h"
#include "../components/tion-api/tion-api-3s.h"
#include "../components/tion-api/tion-api-ble-3s.h"
#include "../components/tion/tion_component.h"
#include "../components/tion_3s_proxy/tion_3s_proxy.h"

#include "test_api.h"
#include "test_vport.h"

DEFINE_TAG;

using dentra::tion::TionTraits;

using Tion3sUartVPortTest = esphome::vport::VPortUARTComponent<Tion3sUartIOTest, Tion3sUartIOTest::frame_spec_type>;
class Tion3sUartVPortApiTest
    : public esphome::tion::TionVPortApi<Tion3sUartIOTest::frame_spec_type, dentra::tion::Tion3sApi> {
 public:
  Tion3sUartVPortApiTest(vport_t *vport)
      : esphome::tion::TionVPortApi<Tion3sUartIOTest::frame_spec_type, dentra::tion::Tion3sApi>(vport) {}

  dentra::tion::TionState &st() { return this->state_; }
};

class Tion3sBleVPortApiTest
    : public esphome::tion::TionVPortApi<Tion3sBleIOTest::frame_spec_type, dentra::tion::Tion3sApi> {
 public:
  Tion3sBleVPortApiTest(vport_t *vport)
      : esphome::tion::TionVPortApi<Tion3sBleIOTest::frame_spec_type, dentra::tion::Tion3sApi>(vport) {}

  dentra::tion::TionState &st() { return this->state_; }
  void state_reset() { this->state_ = {}; }
  void write_state(const dentra::tion::TionState &st) { this->write_state_(st); }
};

class Tion3sBleVPortTest : public esphome::tion::Tion3sBleVPort {
 public:
  Tion3sBleVPortTest(Tion3sBleIOTest *io) : esphome::tion::Tion3sBleVPort(io) {}
  // uint16_t get_state_type() const { return this->state_type_; }
};

class Tion3sTest : public esphome::tion::Tion3sApiComponent {
 public:
  Tion3sTest(dentra::tion::Tion3sApi *api, esphome::tion::TionVPortType vport_type)
      : esphome::tion::Tion3sApiComponent(api, vport_type) {
    // using this_t = typename std::remove_pointer_t<decltype(this)>;
    // api->on_state.template set<this_t, &this_t::on_state>(*this);
  }

  void state_reset() { ESP_LOGE(TAG, "no reset, may fail"); }
};

bool test_api_3s() {
  bool res = true;

  esphome::ble_client::BLEClient client;
  client.set_address(0x112233445566);
  client.connect();

  Tion3sBleIOTest io(&client);
  Tion3sBleVPortTest vport(&io);
  Tion3sBleVPortApiTest api(&vport);

  io.node_state = esphome::esp32_ble_tracker::ClientState::ESTABLISHED;
  // vport.set_persistent_connection(true);

  cloak::setup_and_loop({&vport});

  api.request_state();
  vport.call_loop();
  res &= cloak::check_data("request_state", io, "3D.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.5A");

  api.st().firmware_version = 0xFFFF;
  api.st().gate_position = dentra::tion::TionGatePosition::INDOOR;
  api.write_state(api.st());
  vport.call_loop();
  res &= cloak::check_data("write_state empty", io, "3D.02.00.00.00.00.01.00.00.00.00.00.00.00.00.00.00.00.00.5A");

  api.reset_filter();
  vport.call_loop();
  res &= cloak::check_data("reset_filter", io, "3D.02.00.00.00.00.01.02.00.00.00.00.00.00.00.00.00.00.00.5A");

  api.st().fan_speed = 1;
  api.st().heater_state = true;
  api.st().power_state = true;
  api.st().sound_state = true;
  api.st().target_temperature = 23;
  api.st().filter_time_left = 79 * 3600 * 24;
  // api.st().hours = 14;
  // api.st().minutes = 45;
  api.st().gate_position = dentra::tion::TionGatePosition::OUTDOOR;
  api.write_state(api.st());
  vport.call_loop();
  res &= cloak::check_data("write_state params", io, "3D.02.01.17.02.0B.01.00.00.00.00.00.00.00.00.00.00.00.00.5A");

  auto copy = api.st();
  api.state_reset();
  io.test_data_push("B3.10.21.17.0B.00.00.00.00.4F.00.00.00.00.00.00.00.FF.FF.5A");
  auto act = std::vector<uint8_t>((uint8_t *) &api.st(), (uint8_t *) &api.st() + sizeof(api.st()));
  auto exp = std::vector<uint8_t>((uint8_t *) &copy, (uint8_t *) &copy + sizeof(copy));
  if (!cloak::check_data_(act, exp)) {
    res &= false;
    // copy.dump("copy", api.traits());
    // api.st().dump("orig", api.traits());
  }

  return res;
}

bool test_3s() {
  bool res = true;

  esphome::ble_client::BLEClient client;
  client.set_address(0x112233445566);
  client.connect();

  Tion3sBleIOTest io(&client);
  Tion3sBleVPortTest vport(&io);
  Tion3sBleVPortApiTest api(&vport);

  vport.set_api(&api);

  cloak::setup_and_loop({&vport});

  // cloak::check_data("comp vport_type", comp.get_vport_type(), esphome::tion::TionVPortType::VPORT_BLE);
  // cloak::check_data("vport state_type", vport.get_state_type(), 0x10B3);

  vport.pair();
  io.on_ble_ready();

  return res;
}

bool test_uart_3s() {
  bool res = true;

  std::string state = "21.17.0B.00.00.00.00.4F.00.0E.2D.00.00.00.00.FF.FF";

  esphome::uart::UARTComponent uart(("B3.10." + state + ".5A").c_str());
  Tion3sUartIOTest io(&uart);
  Tion3sUartVPortTest vport(&io);
  Tion3sUartVPortApiTest api(&vport);

  cloak::setup_and_loop({&vport});
  for (int i = 0; i < 5; i++) {
    vport.call_loop();
  }

  auto act = std::vector<uint8_t>((uint8_t *) &api.st(), (uint8_t *) &api.st() + sizeof(api.st()));
  // FIXME transform TionState to orig 3s state
  res &= cloak::check_data_(act, state);

  api.request_state();
  vport.call_loop();
  res &= cloak::check_data("request_state data", uart, "3D.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.5A");

  api.st().heater_state = false;
  printf("0x%04X\n", api.st().filter_time_left);

  api.reset_filter();
  vport.call_loop();
  res &= cloak::check_data("reset_filter data", uart, "3D:02:01:17:02:0A:01:02:00:00:00:00:00:00:00:00:00:00:00:5A");

  return res;
}

bool test_uart_3s_proxy() {
  bool res = true;

  auto inp = "B3.10.00.00.00.00.00.00.00.00.00.0E.00.00.00.00.00.AA.AA.5A";
  auto out = "3D.01.00.00.00.00.00.00.00.00.00.0E.00.00.00.00.00.FF.FF.5A";

  esphome::uart::UARTComponent uart_inp(inp);
  Tion3sUartIOTest io(&uart_inp);
  Tion3sUartVPortTest vport(&io);

  // as additional input source
  Tion3sUartVPortApiTest api(&vport);
  Tion3sTest comp(&api, esphome::tion::TionVPortType::VPORT_UART);

  esphome::uart::UARTComponent uart_out(out);

  esphome::tion::TionVPortApi<esphome::tion::Tion3sUartVPort::frame_spec_type, esphome::tion_3s_proxy::Tion3sApiProxy>
      api_proxy(&vport);

  esphome::tion_3s_proxy::Tion3sBleProxy proxy(&api_proxy, &uart_out);

  cloak::setup_and_loop({&vport, &comp, &proxy});
  for (int i = 0; i < 5; i++) {
    proxy.call_loop();
    vport.call_loop();
  }

  res &= cloak::check_data("inp data", uart_inp, out);
  res &= cloak::check_data("out data", uart_out, inp);

  return res;
}

REGISTER_TEST(test_api_3s);
REGISTER_TEST(test_3s);
REGISTER_TEST(test_uart_3s);
REGISTER_TEST(test_uart_3s_proxy);
