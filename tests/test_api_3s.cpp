#include <cstring>
#include <vector>

#include "esphome/components/vport/vport_ble.h"
#include "../components/tion-api/tion-api-3s.h"
#include "../components/tion-api/tion-api-ble-3s.h"
#include "../components/tion_3s/climate/tion_3s_climate.h"
#include "../components/tion_3s_proxy/tion_3s_proxy.h"

#include "test_api.h"
#include "test_vport.h"

DEFINE_TAG;

using Tion3sUartVPortApiTest = esphome::tion::TionVPortApi<Tion3sUartIOTest::frame_spec_type, dentra::tion::TionApi3s>;
using Tion3sUartVPortTest = esphome::vport::VPortUARTComponent<Tion3sUartIOTest, Tion3sUartIOTest::frame_spec_type>;

using Tion3sBleVPortApiTest = esphome::tion::TionVPortApi<Tion3sBleIOTest::frame_spec_type, dentra::tion::TionApi3s>;

class Tion3sBleVPortTest : public esphome::tion::Tion3sBleVPort {
 public:
  Tion3sBleVPortTest(Tion3sBleIOTest *io) : esphome::tion::Tion3sBleVPort(io) {}
  // uint16_t get_state_type() const { return this->state_type_; }
};

class Tion3sTest : public esphome::tion::Tion3sClimate {
 public:
  Tion3sTest(dentra::tion::TionApi3s *api) : esphome::tion::Tion3sClimate(api) {
    // using this_t = typename std::remove_pointer_t<decltype(this)>;
    // api->on_state.template set<this_t, &this_t::on_state>(*this);
  }

  esphome::tion::TionVPortType get_vport_type() { return this->vport_type_; }

  dentra::tion::tion3s_state_t &state() { return this->state_; };
  void state_reset() { this->state_ = {}; }

  // void on_state(const dentra::tion::tion3s_state_t &state, uint32_t request_id) {
  //   this->state_ = this->state = state;
  //   ESP_LOGD(TAG, "Received tion3s_state_t %s", hexencode_cstr(&state, sizeof(state)));
  //   this->update_state();
  // }
};

bool test_api_3s() {
  bool res = true;

  esphome::ble_client::BLEClient client;
  client.set_address(0x112233445566);
  client.connect();

  Tion3sBleIOTest io(&client);
  Tion3sBleVPortTest vport(&io);
  Tion3sBleVPortApiTest api(&vport);
  Tion3sTest comp(&api);

  io.node_state = esphome::esp32_ble_tracker::ClientState::ESTABLISHED;
  // vport.set_persistent_connection(true);

  cloak::setup_and_loop({&vport, &comp});

  api.request_state();
  res &= cloak::check_data("request_state", io, "3D.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.5A");

  comp.state().firmware_version = 0xFFFF;
  api.write_state(comp.state());
  res &= cloak::check_data("write_state firmware_version", io,
                           "3D.02.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.5A");

  api.reset_filter(comp.state());
  res &= cloak::check_data("write_state firmware_version", io,
                           "3D.02.00.00.00.00.01.02.00.00.00.00.00.00.00.00.00.00.00.5A");

  comp.state().fan_speed = 1;
  comp.state().flags.heater_state = true;
  comp.state().flags.power_state = true;
  comp.state().flags.sound_state = true;
  comp.state().target_temperature = 23;
  comp.state().filter_time = 79;
  comp.state().hours = 14;
  comp.state().minutes = 45;
  comp.state().gate_position = dentra::tion::tion3s_state_t::GATE_POSITION_OUTDOOR;
  api.write_state(comp.state());
  res &= cloak::check_data("write_state params", io, "3D.02.01.17.02.0B.00.00.4F.00.00.00.00.00.00.00.00.00.00.5A");

  auto copy = comp.state();
  comp.state_reset();
  io.test_data_push("B3.10.21.17.0B.00.00.00.00.4F.00.0E.2D.00.00.00.00.FF.FF.5A");
  auto act = std::vector<uint8_t>((uint8_t *) &comp.state(), (uint8_t *) &comp.state() + sizeof(comp.state()));
  auto exp = std::vector<uint8_t>((uint8_t *) &copy, (uint8_t *) &copy + sizeof(copy));
  res &= cloak::check_data_(act, exp);

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
  Tion3sTest comp(&api);

  vport.set_api(&api);
  // vport.set_state_type(api.get_state_type());
  comp.set_vport_type(vport.get_vport_type());

  cloak::setup_and_loop({&vport, &comp});

  cloak::check_data("comp vport_type", comp.get_vport_type(), esphome::tion::TionVPortType::VPORT_BLE);
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
  Tion3sTest comp(&api);

  cloak::setup_and_loop({&vport, &comp});
  for (int i = 0; i < 5; i++) {
    vport.loop();
  }

  auto act = std::vector<uint8_t>((uint8_t *) &comp.state(), (uint8_t *) &comp.state() + sizeof(comp.state()));
  res &= cloak::check_data_(act, state);

  res &= cloak::check_data("request_state call", api.request_state(), true);
  res &= cloak::check_data("request_state data", uart, "3D.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.5A");

  res &= cloak::check_data("send_heartbeat call", api.send_heartbeat(), false);
  res &= cloak::check_data("request_dev_info call", api.request_dev_info(), false);

  comp.state().flags.heater_state = false;
  printf("0x%04X\n", comp.state().filter_time);

  res &= cloak::check_data("reset_filter call", api.reset_filter(comp.state()), true);
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
  Tion3sTest comp(&api);

  esphome::uart::UARTComponent uart_out(out);

  esphome::tion::TionVPortApi<esphome::tion::Tion3sUartVPort::frame_spec_type, esphome::tion_3s_proxy::TionApi3sProxy>
      api_proxy(&vport);
  esphome::tion_3s_proxy::Tion3sProxy proxy(&api_proxy, &uart_out);

  cloak::setup_and_loop({&vport, &comp, &proxy});
  for (int i = 0; i < 5; i++) {
    proxy.loop();
    vport.loop();
  }

  res &= cloak::check_data("inp data", uart_inp, out);
  res &= cloak::check_data("out data", uart_out, inp);

  return res;
}

REGISTER_TEST(test_api_3s);
REGISTER_TEST(test_3s);
REGISTER_TEST(test_uart_3s);
REGISTER_TEST(test_uart_3s_proxy);
