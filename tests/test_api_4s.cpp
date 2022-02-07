
#include "utils.h"
#include "../components/tion-api/tion-api-4s.h"

#include "test_api_4s.h"

using namespace dentra::tion;

static std::vector<uint8_t> wr_data_;

class Api4sTest : public TionApi4s {
 public:
  bool write_data(const uint8_t *data, uint16_t size) const {
    LOGD("Writting data: %s", hexencode(data, size).c_str());
    wr_data_.insert(wr_data_.end(), data, data + size);
    return true;
  }

  uint16_t received_struct_ = {};

  void read(const tion_dev_status_t &dev_status) override {
    received_struct_ = DEV_STATUS;
    LOGD("Received tion_dev_status_t");
  };
  void read(const tion4s_state_t &state) override {
    received_struct_ = STATE;
    LOGD("Received tion_state_t");
  };
  void read(const tion4s_turbo_t &turbo_state) override {
    received_struct_ = TURBO;
    LOGD("Received tion_turbo_state_t");
  };
  void read(const tion4s_time_t &time) override {
    received_struct_ = TIME;
    LOGD("Received tion_time_t");
  };

 protected:
};

bool test_api_4s(bool print) {
  bool res = true;

  for (auto data : test_4s_data) {
    Api4sTest t4s;
    for (auto d : data.frames) {
      t4s.read_data(from_hex(d));
    }
    test_check(res, t4s.received_struct_, data.await_struct);
  }

  wr_data_.clear();
  Api4sTest t4s;
  t4s.request_state();
  test_check(res, wr_data_, from_hex("80.0C.00.3A.AD 32.32 01.00.00.00 64.F7"));

  wr_data_.clear();
  t4s.request_dev_status();
  test_check(res, wr_data_, from_hex("80.0C.00.3A.AD 32.33 01.00.00.00 CE.A6"));

  wr_data_.clear();
  t4s.request_timers();
  test_check(res, wr_data_,
             from_hex("80.11.00.3A.AD 32.34 01.00.00.00 01.00.00.00.00 DB.D5"
                      "80.11.00.3A.AD 32.34 01.00.00.00 01.00.00.00.01 CB.F4"
                      "80.11.00.3A.AD 32.34 01.00.00.00 01.00.00.00.02 FB.97"
                      "80.11.00.3A.AD 32.34 01.00.00.00 01.00.00.00.03 EB.B6"
                      "80.11.00.3A.AD 32.34 01.00.00.00 01.00.00.00.04 9B.51"
                      "80.11.00.3A.AD 32.34 01.00.00.00 01.00.00.00.05 8B.70"));

  return res;
}
