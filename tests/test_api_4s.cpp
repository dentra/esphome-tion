
#include "utils.h"
#include "../components/tion-api/tion-api-4s.h"
#include "test_api_4s.h"
#include "test_vport.h"

using namespace dentra::tion;

static std::vector<uint8_t> wr_data_;

class Api4sTest {
  using this_type = Api4sTest;

 public:
  TionApi4s api;

  explicit Api4sTest(TestTionBleLtProtocol *protocol) {
    protocol->reader.set<Api4sTest, &Api4sTest::read_frame>(*this);
    protocol->writer.set<Api4sTest, &Api4sTest::write_data>(*this);

    this->api.writer.set<TionBleLtProtocol, &TionBleLtProtocol::write_frame>(*protocol);
    this->api.on_state.set<Api4sTest, &Api4sTest::on_state>(*this);
    this->api.on_dev_status.set<Api4sTest, &Api4sTest::on_dev_status>(*this);
    this->api.on_turbo.set<Api4sTest, &Api4sTest::on_turbo>(*this);
    this->api.on_time.set<Api4sTest, &Api4sTest::on_time>(*this);
  }

  uint16_t received_struct_ = {};

  bool write_data(const uint8_t *data, size_t size) {
    LOGD("Writting data: %s", hexencode(data, size).c_str());
    wr_data_.insert(wr_data_.end(), data, data + size);
    return true;
  }

  bool read_frame(uint16_t type, const void *data, size_t size) { return this->api.read_frame(type, data, size); }

  void on_state(const tion4s_state_t &state, uint32_t request_id) {
    this->received_struct_ = STATE;
    LOGD("Received tion_state_t");
  }

  void on_dev_status(const tion_dev_status_t &dev_status) {
    this->received_struct_ = DEV_STATUS;
    LOGD("Received tion_dev_status_t");
  };

  void on_turbo(const tion4s_turbo_t &turbo_state, const uint32_t request_id) {
    this->received_struct_ = TURBO;
    LOGD("Received tion_turbo_state_t");
  };

  void on_time(const time_t time, const uint32_t request_id) {
    this->received_struct_ = TIME;
    LOGD("Received tion_time_t");
  };
};

bool test_api_4s(bool print) {
  bool res = true;

  TestTionBleLtProtocol p;
  for (auto data : test_4s_data) {
    Api4sTest t4s(&p);
    // p.set_api(&t4s);
    for (auto d : data.frames) {
      p.read_data(from_hex(d));
    }
    test_check(res, t4s.received_struct_, data.await_struct);
  }

  Api4sTest t4s(&p);
  // p.set_api(&t4s);

  wr_data_.clear();
  t4s.api.request_state();
  test_check(res, wr_data_, from_hex("80.0C.00.3A.AD 32.32 01.00.00.00 64.F7"));

  wr_data_.clear();
  t4s.api.request_dev_status();
  test_check(res, wr_data_, from_hex("80.0C.00.3A.AD 32.33 01.00.00.00 CE.A6"));

  wr_data_.clear();
  t4s.api.request_timer(0, 1);
  test_check(res, wr_data_, from_hex("80.11.00.3A.AD 32.34 01.00.00.00 01.00.00.00.00 DB.D5"));

  wr_data_.clear();
  t4s.api.request_timers(1);
  test_check(res, wr_data_,
             from_hex("80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.00.DB.D5"
                      "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.01.CB.F4"
                      "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.02.FB.97"
                      "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.03.EB.B6"
                      "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.04.9B.51"
                      "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.05.8B.70"
                      "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.06.BB.13"
                      "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.07.AB.32"
                      "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.08.5A.DD"
                      "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.09.4A.FC"
                      "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.0A.7A.9F"
                      "80.11.00.3A.AD.32.34.01.00.00.00.01.00.00.00.0B.6A.BE"));

  return res;
}
