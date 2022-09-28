#include <cstring>
#include <vector>
#include "utils.h"

#include "../components/tion-api/tion-api-3s.h"
#include "../components/tion-api/tion-api-ble-3s.h"
#include "test_api.h"
using namespace dentra::tion;

static std::vector<uint8_t> wr_data_;

class Api3sTest {
  using this_type = Api3sTest;

 public:
  TionsApi3s api;
  tion3s_state_t state{};

  Api3sTest(TestTionBle3sProtocol *protocol) {
    protocol->reader.set<Api3sTest, &Api3sTest::read_frame>(*this);
    protocol->writer.set<Api3sTest, &Api3sTest::write_data>(*this);

    this->api.writer.set<TionBle3sProtocol, &TionBle3sProtocol::write_frame>(*protocol);
    this->api.on_state.set<Api3sTest, &Api3sTest::on_state>(*this);
  }

  bool read_frame(uint16_t type, const void *data, size_t size) { return this->api.read_frame(type, data, size); }

  bool write_data(const uint8_t *data, size_t size) {
    LOGD("Writting data: %s", hexencode(data, size).c_str());
    wr_data_.insert(wr_data_.end(), data, data + size);
    return true;
  }

  void on_state(const tion3s_state_t &state, uint32_t request_id) {
    LOGD("Received tion3s_state_t %s", hexencode(&state, sizeof(state)).c_str());
    this->log_state(state);
    this->state = state;
    return;
  }

  void log_state(const tion3s_state_t &st) {
    LOGD("fan_speed=%u, gate_position=%u, target_temperature=%d, heater_state=%s, power_state=%s, timer_state=%s, "
         "sound_state=%s, auto_state=%s, ma_connect=%s, save=%s, ma_pairing=%s, preset_state=%s, "
         "reserved=0x%02X, unknown_temperature=%d, outdoor_temperature=%d, current_temperature=%d, filter_time=%u, "
         "hours=%u, minutes=%u, last_error=%u, productivity=%u, filter_days=%u, firmware_version=%04X",
         st.fan_speed, st.gate_position, st.target_temperature, ONOFF(st.flags.heater_state),
         ONOFF(st.flags.power_state), ONOFF(st.flags.timer_state), ONOFF(st.flags.sound_state),
         ONOFF(st.flags.auto_state), ONOFF(st.flags.ma_connect), ONOFF(st.flags.save), ONOFF(st.flags.ma_pairing),
         ONOFF(st.flags.preset_state), st.flags.reserved, st.unknown_temperature, st.outdoor_temperature,
         st.current_temperature, st.filter_time, st.hours, st.minutes, st.last_error, st.productivity, st.filter_days,
         st.firmware_version);
  }
};

bool test_api_3s(bool print) {
  bool res = true;

  TestTionBle3sProtocol p;
  Api3sTest test(&p);

  wr_data_.clear();
  test.api.request_state();
  test_check(res, wr_data_, from_hex("3D.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.5A"));

  test.state.firmware_version = 0xFFFF;

  wr_data_.clear();
  test.api.write_state(test.state);
  test_check(res, wr_data_, from_hex("3D.02.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.5A"));

  wr_data_.clear();
  test.api.reset_filter(test.state);
  test_check(res, wr_data_, from_hex("3D.02.00.00.00.00.00.02.00.00.00.00.00.00.00.00.00.00.00.5A"));

  wr_data_.clear();
  test.state.fan_speed = 1;
  test.state.flags.heater_state = true;
  test.state.flags.power_state = true;
  test.state.flags.sound_state = true;
  test.state.target_temperature = 23;
  test.state.filter_time = 79;
  test.state.hours = 14;
  test.state.minutes = 45;
  test.state.gate_position = tion3s_state_t::GATE_POSITION_OUTDOOR;
  test.api.write_state(test.state);
  test_check(res, wr_data_, from_hex("3D.02.01.17.02.0B.00.00.4F.00.00.00.00.00.00.00.00.00.00.5A"));

  tion3s_state_t copy = test.state;
  test.state = {};
  test_check(res, p.read_data(from_hex("B3.10.21.17.0B.00.00.00.00.4F.00.0E.2D.00.00.00.00.FF.FF.5A")), true);
  test_check(res, std::vector<uint8_t>((uint8_t *) &test.state, (uint8_t *) &test.state + sizeof(test.state)),
             std::vector<uint8_t>((uint8_t *) &copy, (uint8_t *) &copy + sizeof(copy)));

  return res;
}
