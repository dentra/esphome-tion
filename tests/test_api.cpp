

#include "utils.h"
#include "../components/tion-api/log.h"
#include "../components/tion-api/tion-api.h"
#include "../components/tion-api/tion-api-4s.h"
#include "test_api.h"
#include "test_api_4s.h"

using namespace dentra::tion;

// static std::vector<uint8_t> wr_data_;

bool BleProtocolTest::write_data(const uint8_t *data, size_t size) const {
  LOGD("Writting data: %s", hexencode(data, size).c_str());
  return true;
}

class ApiTest : public dentra::tion::TionApi<tion4s_state_t> {
 public:
  ApiTest(BleProtocolTest *w) : TionApi(w) {}
  uint16_t get_state_type() const override { return 0; }
  bool request_dev_status() const override { return false; }
  bool request_state() const override { return false; }

  uint16_t received_frame_type = {};
  uint16_t received_frame_size = {};

  bool read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) override {
    LOGD("Received frame data 0x%04X: %s", frame_type, hexencode(frame_data, frame_data_size).c_str());
    received_frame_type = frame_type;
    received_frame_size = frame_data_size;
    return true;
  }
};

bool test_api(bool print) {
  bool res = true;

  BleProtocolTest p;
  for (auto data : test_4s_data) {
    ApiTest t(&p);
    p.set_api(&t);
    for (auto d : data.frames) {
      p.read_data(from_hex(d));
    }
    test_check(res, t.received_frame_type, data.await_frame_type);
    test_check(res, t.received_frame_size, data.await_frame_size);
  }

  return res;
}
