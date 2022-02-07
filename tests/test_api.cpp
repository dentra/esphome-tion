

#include "utils.h"
#include "../components/tion-api/log.h"
#include "../components/tion-api/tion-api.h"

#include "test_api.h"
#include "test_api_4s.h"

using namespace dentra::tion;

// static std::vector<uint8_t> wr_data_;

class ApiTest : public TionApi {
 public:
  bool write_data(const uint8_t *data, uint16_t size) const {
    LOGD("Writting data: %s", hexencode(data, size).c_str());
    // wr_data_.clear();
    // wr_data_.insert(wr_data_.end(), data, data + size);
    return true;
  }

  uint16_t received_frame_type = {};
  uint16_t received_frame_size = {};

  void request_dev_status() override{};
  void request_state() override{};

 protected:
  void read_(uint16_t frame_type, const void *frame_data, uint16_t frame_data_size) override {
    LOGD("Received frame data 0x%04X: %s", frame_type, hexencode(frame_data, frame_data_size).c_str());
    received_frame_type = frame_type;
    received_frame_size = frame_data_size;
  }
};

bool test_api(bool print) {
  bool res = true;

  for (auto data : test_4s_data) {
    ApiTest t;
    for (auto d : data.frames) {
      t.read_data(from_hex(d));
    }
    test_check(res, t.received_frame_type, data.await_frame_type);
    test_check(res, t.received_frame_size, data.await_frame_size);
  }

  return res;
}
