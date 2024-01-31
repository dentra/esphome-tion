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

auto states = {
    "11 0C 0C 13 10 02 3C 04 00 00 B4 D6 DC 01 01 F6 CA 01 54",
    // "11 0C 0F 15 10 02 3C 04 00 00 B4 D6 DC 01 01 F6 CA 01 51",
    // "11 0C 11 16 10 02 3C 04 00 00 B4 D6 DC 01 01 F6 CA 01 4C",
    // "11 0C FF 0D 10 04 78 04 00 00 F0 D6 DC 01 89 F5 CA 01 34",
    // "11 0C FD 10 10 04 78 04 00 00 2C D7 DC 01 11 F5 CA 01 6E",
    // "11 0C FE 0F 10 02 3C 04 00 00 68 D7 DC 01 99 F4 CA 01 FD",
    // "11 0C FE 0D 0A 02 3C 04 00 00 E0 D7 DC 01 21 F4 CA 01 D5",
    // "11 0E 00 0A 0A 02 3C 04 00 00 E0 99 DE 01 21 32 C9 01 A7",
    // "11 0E 00 0A 0A 02 3C 04 00 00 34 A0 DE 01 CD 2B C9 01 BF",
    // "11 0E FF 09 0A 03 4B 04 00 00 00 A7 DE 01 F2 24 C9 01 36",
    // // "11 0E FF 0A 0A 04 78 04 00 00 68 A8 DE 01    30 C9 01 A3",
    // "11 0E FF 0A 0A 01 23 04 00 00 1C A9 DE 01 C8 21 C9 01 72",
    // "11 0C FF 08 0A 01 23 04 00 00 C0 AA DE 01 D3 20 C9 01 B7",
    "11 0E 03 09 0A 01 23 04 00 00 C0 AA DE 01 D3 20 C9 01 48",
    "11 06 00 07 0A 01 23 04 00 00 64 AC DE 01 DE 1F C9 01 DD",
    // "11 0E FF 06 0A 01 23 04 00 00 DC AC DE 01 98 1F C9 01 D5",
    // "11 0E FF 07 0B 01 23 04 00 00 90 AD DE 01 2F 1F C9 01 2F",
    // "11 0E FF 07 19 01 23 04 00 00 08 AE DE 01 E9 1E C9 01 61",
    // "11 0E FF 0A EC 01 23 04 00 00 80 AE DE 01 A3 1E C9 01 5B",
    // "11 0C 04 06 EC 01 23 04 00 00 14 B1 DE 01 22 1D C9 01 A7",
};

static char bits_str_buf[9]{};
const char *bits_str(uint8_t v) {
  for (int i = 0; i < 8; i++) {
    bits_str_buf[7 - i] = ((v >> i) & 1) + '0';
  }
  return bits_str_buf;
}

#define DUMP_UNK(field) \
  if (state.field == 0 || state.field == 1) \
    ESP_LOGD(TAG, "%-12s: %u", #field, state.field); \
  else if (static_cast<int8_t>(state.field) > 0) \
    ESP_LOGD(TAG, "%-12s: 0x%02X, %s, %u", #field, state.field, bits_str(state.field), state.field); \
  else \
    ESP_LOGD(TAG, "%-12s: 0x%02X, %s, %u, %d", #field, state.field, bits_str(state.field), state.field, \
             static_cast<int8_t>(state.field));

bool test_api_o2_data() {
  bool res = true;
  // -25 °C, -30 °C, -35 °C, -40 °C
  // for (int i = 25; i <= 40; i += 5) {
  //   printf("turn off temp: %d, 0x%02x\n", -i, ((uint16_t) -i) & 0xFF);
  // }
  // for (int i = 30; i <= 360; i += 30) {
  //   auto s = i * 24 * 3600;
  //   printf("max filter days: %3u, 0x%04x, 0x%08x\n", i, i, s);
  // }

  for (auto hex : states) {
    auto raw = cloak::from_hex(hex);
    ESP_LOGD(TAG, "checking packet %s", hexencode_cstr(raw));
    auto state_ptr = reinterpret_cast<const dentra::tion_o2::tiono2_state_t *>(raw.data() + 1);
    auto &state = *state_ptr;
    ESP_LOGD(TAG, "flags       : %s", bits_str(*reinterpret_cast<const uint8_t *>(&state.flags)));
    ESP_LOGD(TAG, "flags.power : %s", ONOFF(state.flags.power_state));
    ESP_LOGD(TAG, "flags.heater: %s", ONOFF(state.flags.heater_state));
    ESP_LOGD(TAG, "outdoor_temp: %d", state.outdoor_temperature);
    ESP_LOGD(TAG, "current_temp: %d", state.current_temperature);
    ESP_LOGD(TAG, "target_temp : %d", state.current_temperature);
    ESP_LOGD(TAG, "fan_speed   : %d", state.fan_speed);
    ESP_LOGD(TAG, "productivity: %d", state.productivity);
    DUMP_UNK(unknown7);
    DUMP_UNK(unknown8);
    DUMP_UNK(unknown9);
    ESP_LOGD(TAG, "work_time   : %" PRIu32, state.counters.work_time_days());
    ESP_LOGD(TAG, "filter_time : %" PRIu32, state.counters.filter_time_left());

    printf("346 = 0x%08X\n", 346 * 24 * 3600);
    printf("347 = 0x%08X\n", 347 * 24 * 3600);
  }

  return res;
}

REGISTER_TEST(test_api_o2);
REGISTER_TEST(test_api_o2_proxy);
REGISTER_TEST(test_api_o2_data);
