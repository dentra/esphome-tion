#include <cstring>
#include <vector>
#include "utils.h"

#include "test_api.h"
#include "../components/tion-api/tion-api-3s.h"

using namespace dentra::tion;

static std::vector<uint8_t> wr_data_;

class Api3sTest : public TionsApi3s {
 public:
  bool write_data(const uint8_t *data, uint16_t size) const override {
    LOGD("Writting data: %s", hexencode(data, size).c_str());
    wr_data_.insert(wr_data_.end(), data, data + size);
    return true;
  }

  tion3s_state_t state{};
  void read(const tion3s_state_t &state) {
    LOGD("Received tion3s_state_t %s", hexencode(&state, sizeof(state)).c_str());
    this->log_state(state);
    this->state = state;
    return;
  }

  void log_state(const tion3s_state_t &st) {
    LOGD("fan_speed=%u, gate_position=%u, target_temperature=%d, heater_state=%s, power_state=%s, timer_state=%s, "
         "sound_state=%s, auto_state=%s, ma_connect=%s, save=%s, ma_pairing=%s, preset_state=%s, "
         "reserved=0x%02X, outdoor_temperature1=%d, outdoor_temperature2=%d, indoor_temperature=%d, filter_time=%u, "
         "hours=%u, minutes=%u, last_error=%u, productivity=%u, filter_days=%u, firmware_version=%04X",
         st.fan_speed, st.gate_position, st.target_temperature, ONOFF(st.flags.heater_state),
         ONOFF(st.flags.power_state), ONOFF(st.flags.timer_state), ONOFF(st.flags.sound_state),
         ONOFF(st.flags.auto_state), ONOFF(st.flags.ma_connect), ONOFF(st.flags.save), ONOFF(st.flags.ma_pairing),
         ONOFF(st.flags.preset_state), st.flags.reserved, st.outdoor_temperature1, st.outdoor_temperature2,
         st.indoor_temperature, st.filter_time, st.hours, st.minutes, st.last_error, st.productivity, st.filter_days,
         st.firmware_version);
  }
};

bool test_api_3s(bool print) {
  bool res = true;

  Api3sTest api;

  wr_data_.clear();
  api.request_state();
  test_check(res, wr_data_, from_hex("3D.01.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.5A"));

  api.state.firmware_version = 0xFFFF;

  wr_data_.clear();
  api.write_state(api.state);
  test_check(res, wr_data_, from_hex("3D.02.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.5A"));

  wr_data_.clear();
  api.reset_filter(api.state);
  test_check(res, wr_data_, from_hex("3D.02.00.00.00.00.00.02.00.00.00.00.00.00.00.00.00.00.00.5A"));

  wr_data_.clear();
  api.state.fan_speed = 1;
  api.state.flags.heater_state = true;
  api.state.flags.power_state = true;
  api.state.flags.sound_state = true;
  api.state.target_temperature = 23;
  api.state.filter_time = 79;
  api.state.hours = 14;
  api.state.minutes = 45;
  api.state.gate_position = tion3s_state_t::GATE_POSITION_OUTDOOR;
  api.write_state(api.state);
  test_check(res, wr_data_, from_hex("3D.02.01.17.02.0B.00.00.4F.00.00.00.00.00.00.00.00.00.00.5A"));

  tion3s_state_t copy = api.state;
  api.state = {};
  test_check(res, api.read_data(from_hex("B3.10.21.17.0B.00.00.00.00.4F.00.0E.2D.00.00.00.00.FF.FF.5A")), true);
  test_check(res, std::vector<uint8_t>((uint8_t *) &api.state, (uint8_t *) &api.state + sizeof(api.state)),
             std::vector<uint8_t>((uint8_t *) &copy, (uint8_t *) &copy + sizeof(copy)));

  return res;
}
