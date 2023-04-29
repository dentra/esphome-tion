#include <cstring>
#include <vector>

#include "esphome/components/vport/vport_ble.h"
#include "../components/tion-api/tion-api-3s.h"
#include "../components/tion-api/tion-api-ble-3s.h"
#include "../components/tion_3s/tion_3s.h"

#include "test_api.h"
#include "test_vport.h"

DEFINE_TAG;

using Tion3sUartVPortApiTest = esphome::tion::TionVPortApi<Tion3sUartIOTest::frame_spec_type, dentra::tion::TionApi3s>;
using Tion3sUartVPortTest = esphome::vport::VPortUARTComponent<Tion3sUartIOTest, Tion3sUartIOTest::frame_spec_type>;

using Tion3sBleVPortApiTest = esphome::tion::TionVPortApi<Tion3sBleIOTest::frame_spec_type, dentra::tion::TionApi3s>;

class Tion3sBleVPortTest : public esphome::tion::Tion3sBleVPort {
 public:
  Tion3sBleVPortTest(Tion3sBleIOTest *io) : esphome::tion::Tion3sBleVPort(io) {}
  uint16_t get_state_type() const { return this->state_type_; }
};

class Tion3sTest : public esphome::tion::Tion3s {
 public:
  Tion3sTest(dentra::tion::TionApi3s *api) : esphome::tion::Tion3s(api) {
    using this_t = typename std::remove_pointer_t<decltype(this)>;
    api->on_state.template set<this_t, &this_t::on_state>(*this);
  }

  esphome::tion::TionVPortType get_vport_type() { return this->vport_type_; }

  dentra::tion::tion3s_state_t state{};

  void on_state(const dentra::tion::tion3s_state_t &state, uint32_t request_id) {
    ESP_LOGD(TAG, "Received tion3s_state_t %s", hexencode_cstr(&state, sizeof(state)));
    this->log_state(state);
    this->state = state;
  }

  void log_state(const dentra::tion::tion3s_state_t &st) {
    ESP_LOGD(TAG,
             "fan_speed=%u, gate_position=%u, target_temperature=%d, heater_state=%s, power_state=%s, timer_state=%s, "
             "sound_state=%s, auto_state=%s, ma_connect=%s, save=%s, ma_pairing=%s, preset_state=%s, "
             "reserved=0x%02X, unknown_temperature=%d, outdoor_temperature=%d, current_temperature=%d, filter_time=%u, "
             "hours=%u, minutes=%u, last_error=%u, productivity=%u, filter_days=%u, firmware_version=%04X",
             st.fan_speed, st.gate_position, st.target_temperature, ONOFF(st.flags.heater_state),
             ONOFF(st.flags.power_state), ONOFF(st.flags.timer_state), ONOFF(st.flags.sound_state),
             ONOFF(st.flags.auto_state), ONOFF(st.flags.ma_connect), ONOFF(st.flags.save), ONOFF(st.flags.ma_pairing),
             ONOFF(st.flags.preset_state), st.flags.reserved, st.unknown_temperature, st.outdoor_temperature,
             st.current_temperature, st.filter_time, st.hours, st.minutes, st.last_error, st.productivity,
             st.filter_days, st.firmware_version);
  };
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

  comp.state.firmware_version = 0xFFFF;
  api.write_state(comp.state);
  res &= cloak::check_data("write_state firmware_version", io,
                           "3D.02.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.5A");

  api.reset_filter();
  res &= cloak::check_data("write_state firmware_version", io,
                           "3D.04.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.5A");

  comp.state.fan_speed = 1;
  comp.state.flags.heater_state = true;
  comp.state.flags.power_state = true;
  comp.state.flags.sound_state = true;
  comp.state.target_temperature = 23;
  comp.state.filter_time = 79;
  comp.state.hours = 14;
  comp.state.minutes = 45;
  comp.state.gate_position = dentra::tion::tion3s_state_t::GATE_POSITION_OUTDOOR;
  api.write_state(comp.state);
  res &= cloak::check_data("write_state params", io, "3D.02.01.17.02.0B.00.00.4F.00.00.00.00.00.00.00.00.00.00.5A");

  auto copy = comp.state;
  comp.state = {};
  io.test_data_push("B3.10.21.17.0B.00.00.00.00.4F.00.0E.2D.00.00.00.00.FF.FF.5A");
  auto act = std::vector<uint8_t>((uint8_t *) &comp.state, (uint8_t *) &comp.state + sizeof(comp.state));
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
  vport.set_state_type(api.get_state_type());
  comp.set_vport_type(vport.get_vport_type());

  cloak::setup_and_loop({&vport, &comp});

  cloak::check_data("comp vport_type", comp.get_vport_type(), esphome::tion::TionVPortType::VPORT_BLE);
  cloak::check_data("vport state_type", vport.get_state_type(), 0x10B3);

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

  auto act = std::vector<uint8_t>((uint8_t *) &comp.state, (uint8_t *) &comp.state + sizeof(comp.state));
  res &= cloak::check_data_(act, state);

  res &= cloak::check_data("request_state call", api.request_state(), true);
  res &= cloak::check_data("request_state data", uart, "3D.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.5A");

  res &= cloak::check_data("send_heartbeat call", api.send_heartbeat(), false);
  res &= cloak::check_data("request_dev_status call", api.request_dev_status(), false);

  res &= cloak::check_data("reset_filter call", api.reset_filter(), true);
  res &= cloak::check_data("reset_filter data", uart, "3D.04.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.5A");

  return res;
}

REGISTER_TEST(test_api_3s);
REGISTER_TEST(test_3s);
REGISTER_TEST(test_uart_3s);
