#include <cstring>
#include <vector>
#include "utils.h"

#include "../components/tion-api/tion-api-3s.h"
#include "../components/tion-api/tion-api-ble-3s.h"

using namespace dentra::tion;

static std::vector<uint8_t> wr_data_;

class Ble3sProtocolTest : public dentra::tion::TionBle3sProtocol {
 public:
  bool write_data(const uint8_t *data, size_t size) const override {
    LOGD("Writting data: %s", hexencode(data, size).c_str());
    wr_data_.insert(wr_data_.end(), data, data + size);
    return true;
  }
  bool read_data(const std::vector<uint8_t> &data) {
    return dentra::tion::TionBle3sProtocol::read_data(data.data(), data.size());
  }
  bool read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) override {
    return this->reader_->read_frame(frame_type, frame_data, frame_data_size);
  }
  template<class T, typename std::enable_if<std::is_base_of<TionFrameReader, T>::value, bool>::type = true>
  void set_api(T *api) {
    this->reader_ = api;
  }

 protected:
  dentra::tion::TionFrameReader *reader_{};
};

class Api3sTest : public TionsApi3s {
 public:
  Api3sTest(Ble3sProtocolTest *writer) : TionsApi3s(writer) {}
  uint16_t get_state_type() const override { return 0; }
  bool request_dev_status() const override { return true; }

  tion3s_state_t state{};
  void on_state(const tion3s_state_t &state, uint32_t request_id) override {
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

  Ble3sProtocolTest p;
  Api3sTest api(&p);
  p.set_api(&api);

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
  test_check(res, p.read_data(from_hex("B3.10.21.17.0B.00.00.00.00.4F.00.0E.2D.00.00.00.00.FF.FF.5A")), true);
  test_check(res, std::vector<uint8_t>((uint8_t *) &api.state, (uint8_t *) &api.state + sizeof(api.state)),
             std::vector<uint8_t>((uint8_t *) &copy, (uint8_t *) &copy + sizeof(copy)));

  return res;
}
