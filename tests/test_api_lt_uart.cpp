#include "utils.h"

#include "esphome/components/uart/uart_component.h"

#include "../components/tion-api/tion-api-uart-lt.h"
#include "../components/tion-api/tion-api-lt-internal.h"
#include "../components/tion-api/tion-api-lt.h"
DEFINE_TAG;

using esphome::uart::UARTComponent;
using dentra::tion::TionState;
using dentra::tion::TionLtApi;
using namespace dentra::tion_lt;

class TestTionLtUartReader : public dentra::tion::TionUartReader {
 public:
  TestTionLtUartReader(UARTComponent *uart) : uart_(uart) {}
  int available() override { return this->uart_->available(); }
  bool read_array(void *data, size_t size) override { return this->uart_->read_array(data, size); }

 protected:
  UARTComponent *uart_;
};

bool test_api_lt_uart() {
  bool res = true;
  TionLtUartProtocol pr;
  pr.writer = [](const uint8_t *data, size_t size) {
    ESP_LOGD(TAG, "GOT TX: %s (%zu)", data, size);
    return true;
  };

  pr.write_frame(FRAME_TYPE_DEV_INFO_REQ, nullptr, 0);
  pr.write_frame(FRAME_TYPE_STATE_REQ, nullptr, 0);

  TionState state{};
  state.fan_speed = 3;
  state.heater_state = true;
  state.power_state = true;
  state.target_temperature = 23;
  tionlt_state_set_req_t st_set(state, button_presets_t{}, 0);
  pr.write_frame(FRAME_TYPE_STATE_SET, &st_set, sizeof(st_set));

  TionLtApi api;
  pr.reader = [&api](const TionLtUartProtocol::frame_spec_type &frame, size_t size) {
    api.read_frame(frame.type, frame.data, size - TionLtUartProtocol::frame_spec_type::head_size());
  };

  const char state_data[] = "\r\n"
                            "Switching Mode\r\n"
                            "Current Mode: Work\r\n"
                            "Speed: 2\r\n"
                            "Sensors T_set: 12, T_In: -23, T_out: 24\r\n"
                            "PID_Value: 17 1\r\n"
                            "Filter Time: 10984449\r\n"
                            "Working Time: 27853548\r\n"
                            "Power On Time: 81257423\r\n"
                            "Error register: 255\r\n"
                            "MAC: 247 74 249 223 116 211\r\n"
                            "Firmware Version 0x054B\r\n"
                            "\r\n";
  UARTComponent uart(reinterpret_cast<const uint8_t *>(state_data), sizeof(state_data) - 1);
  TestTionLtUartReader io(&uart);
  for (int i = 0; i < 20; i++) {
    pr.read_uart_data(&io);
  }

  static const uint8_t PROD[] = {0, TION_LT_AUTO_PROD};
  // speed 2, fan time
  // 27849498
  // 27851575
  // 27851580
  // 27851613
  // 27853548
  const uint32_t fan_time_for_speed1[] = {27849036, 27849051, 27849062, 27849077, 27849092, 27849121,
                                          27849129, 27849141, 27849142, 27849160, 27849183, 27849203};
  const uint32_t fan_time_for_speed2[] = {27849215, 27849234, 27849242, 27849253, 27849270, 27849284,
                                          27849303, 27849308, 27849328, 27849363, 27849468, 27849498};
#define fan_time_for_speed fan_time_for_speed2
#define speed_for_fan_time 2
  uint32_t prev_ft = 0, prev_ac = 0;
  for (int i = 0; i < sizeof(fan_time_for_speed) / sizeof(fan_time_for_speed[0]); i++) {
    dentra::tion_lt::tion_lt_state_counters_t cnt{};
    cnt.fan_time = fan_time_for_speed[i];

    const uint32_t dif_ft = cnt.fan_time - prev_ft;
    // cnt.airflow_counter = (diff_fan_time * (PROD[2] * (3600 / 10))) * dif_ft;
    const uint32_t dif_ac = dif_ft * (PROD[speed_for_fan_time] / dentra::tion_lt::tion_lt_state_counters_t::AK);
    cnt.airflow_counter = prev_ac + dif_ac;
    // dif_ac/dif_ft*10=30
    // dif_ac/dif_ft=30/10
    // dif_ac=30/10*dif_ft
    // dif_ac=pre_ac-cur_ac

    ESP_LOGD(TAG, "prod[%d] %u, diff: %u", i, cnt.calc_productivity(prev_ft, prev_ac), dif_ft);
    prev_ft = cnt.fan_time;
    prev_ac = cnt.airflow_counter;
  }

  return true;
}

REGISTER_TEST(test_api_lt_uart);
