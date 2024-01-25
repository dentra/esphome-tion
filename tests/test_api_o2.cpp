#include <cstring>
#include <vector>

#include "esphome/components/vport/vport_uart.h"
#include "../components/tion-api/tion-api-o2.h"
#include "../components/tion-api/tion-api-uart-o2.h"
#include "../components/tion_o2/climate/tion_o2_climate.h"
#include "../components/tion_o2/vport/tion_o2_vport.h"
#include "../components/tion_o2_proxy/tion_o2_proxy.h"

#include "test_api.h"
#include "utils.h"

DEFINE_TAG;

using TionO2UartIOTest = esphome::tion::TionO2UartIO;
using TionO2UartVPortApiTest =
    esphome::tion::TionVPortApi<TionO2UartIOTest::frame_spec_type, dentra::tion_o2::TionO2Api>;
using TionO2UartVPortTest = esphome::vport::VPortUARTComponent<TionO2UartIOTest, TionO2UartIOTest::frame_spec_type>;

class TionO2Test : public esphome::tion::TionO2Climate {
 public:
  TionO2Test(dentra::tion_o2::TionO2Api *api, esphome::tion::TionVPortType vport_type)
      : esphome::tion::TionO2Climate(api, vport_type) {}

  esphome::tion::TionVPortType get_vport_type() const { return this->vport_type_; }

  dentra::tion_o2::tiono2_state_t &state() { return this->state_; };
  void state_reset() { this->state_ = {}; }

  // void on_state(const dentra::tion::tion3s_state_t &state, uint32_t request_id) {
  //   this->state_ = this->state = state;
  //   ESP_LOGD(TAG, "Received tion3s_state_t %s", hexencode_cstr(&state, sizeof(state)));
  //   this->update_state();
  // }
};

bool test_api_o2() {
  bool res = true;

  esphome::uart::UARTComponent uart("11 0C 0C 13 10 02 3C 04 00 00 B4 D6 DC 01 01 F6 CA 01 54");
  TionO2UartIOTest io(&uart);
  TionO2UartVPortTest vport(&io);
  TionO2UartVPortApiTest api(&vport);
  // TionO2Test comp(&api, esphome::tion::TionVPortType::VPORT_UART);

  cloak::setup_and_loop({&vport /*, &comp*/});
  for (int i = 0; i < 5; i++) {
    vport.call_loop();
  }

  // auto act = std::vector<uint8_t>((uint8_t *) &comp.state(), (uint8_t *) &comp.state() + sizeof(comp.state()));
  // res &= cloak::check_data_(act, state);

  // res &= cloak::check_data("request_state call", api.request_state(), true);
  // vport.call_loop();
  // res &= cloak::check_data("request_state data", uart,
  // "3D.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.5A");

  return res;
}

bool test_api_o2_proxy() {
  bool res = true;

  auto inp = "11 0C 0C 13 10 02 3C 04 00 00 B4 D6 DC 01 01 F6 CA 01 54";
  auto out = "01 FE";

  esphome::uart::UARTComponent uart_inp(inp);
  TionO2UartIOTest io(&uart_inp);
  TionO2UartVPortTest vport(&io);

  // as additional input source
  TionO2UartVPortApiTest api(&vport);
  TionO2Test comp(&api, esphome::tion::TionVPortType::VPORT_UART);

  esphome::uart::UARTComponent uart_out(out);

  esphome::tion::TionVPortApi<esphome::tion::TionO2UartVPort::frame_spec_type, esphome::tion_o2_proxy::TionO2ApiProxy>
      api_proxy(&vport);
  esphome::tion_o2_proxy::TionO2Proxy proxy(&api_proxy, &uart_out);

  cloak::setup_and_loop({&vport, &comp, &proxy});

  res &= cloak::check_data("heartbeat data", uart_inp, "07 F8");

  for (int i = 0; i < 5; i++) {
    proxy.call_loop();
    vport.call_loop();
  }

  res &= cloak::check_data("inp data", uart_inp, out);
  res &= cloak::check_data("out data", uart_out, inp);

  return res;
}

REGISTER_TEST(test_api_o2);
REGISTER_TEST(test_api_o2_proxy);
