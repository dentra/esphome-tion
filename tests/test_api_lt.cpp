#include "test_api.h"
#include "test_vport.h"

DEFINE_TAG;

using namespace dentra::tion;

class TionLtBleVPortApiTest
    : public esphome::tion::TionVPortApi<TionLtBleIOTest::frame_spec_type, dentra::tion::TionLtApi> {
 public:
  TionLtBleVPortApiTest(vport_t *vport)
      : esphome::tion::TionVPortApi<TionLtBleIOTest::frame_spec_type, dentra::tion::TionLtApi>(vport) {
    this->state_.firmware_version = 0xFFFF;
    this->set_button_presets({20, 20, 20, 01, 03, 05});
  }
  bool request_dev_info() const { return this->request_dev_info_(); }
};

using TionLtBleVPortTest = esphome::tion::TionLtBleVPort;
// class TionLtBleVPortTest : public esphome::tion::TionLtBleVPort {
//  public:
//   TionLtBleVPortTest(TionLtBleIOTest *io) : esphome::tion::TionLtBleVPort(io) {}
// };

bool test_api_lt() {
  bool res = true;

  esphome::ble_client::BLEClient client;
  client.set_address(0x112233445566);
  client.connect();

  TionLtBleIOTest io(&client);
  TionLtBleVPortTest vport(&io);
  TionLtBleVPortApiTest api(&vport);

  vport.set_persistent_connection(true);
  client.enabled = true;
  io.node_state = esphome::esp32_ble_tracker::ClientState::ESTABLISHED;

  cloak::setup_and_loop({&vport});

  api.request_state();
  vport.call_loop();
  res &= cloak::check_data("request_state", io, "80.0C.00.3A.AD.32.12.01.00.00.00.6C.43");

  api.request_dev_info();
  vport.call_loop();
  res &= cloak::check_data("request_dev_info", io, "80.0C.00.3A.AD.09.40.01.00.00.00.D1.DC");

  TionState st{};
  st.power_state = true;
  st.heater_state = true;
  st.fan_speed = 2;
  st.work_time = 0xFFFFFFFF;
  st.filter_time_left = 0xEEEEEEEE;

  api.write_state(st, 1);
  vport.call_loop();
  res &= cloak::check_data("write_state", io,
                           "00.1E.00.3A.AD."
                           "30.12."
                           "01.00.00.00."
                           "01.00.00.00."
                           "11.00.02.00.02."
                           "C0."
                           "14.14.14.01.03.05."  // button preset data
                           "00.00.00."
                           "81.03");

  return res;
}

REGISTER_TEST(test_api_lt);
