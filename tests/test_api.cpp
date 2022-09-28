

#include "utils.h"
#include "../components/tion-api/log.h"
#include "../components/tion-api/tion-api.h"
#include "../components/tion-api/tion-api-4s.h"
#include "test_api.h"
#include "test_api_4s.h"

using namespace dentra::tion;

// static std::vector<uint8_t> wr_data_;

class ApiTest {
 public:
  dentra::tion::TionApiBase<tion4s_state_t> api_;

  ApiTest(TestTionBleLtProtocol *w) {
    w->reader.set<ApiTest, &ApiTest::read_frame>(*this);
    w->writer.set<ApiTest, &ApiTest::write_data>(*this);
  }

  bool write_data(const uint8_t *data, size_t size) {
    LOGD("Writting data: %s", hexencode(data, size).c_str());
    // wr_data_.insert(wr_data_.end(), data, data + size);
    return true;
  }

  uint16_t received_frame_type = {};
  uint16_t received_frame_size = {};

  bool read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
    LOGD("Received frame data 0x%04X: %s", frame_type, hexencode(frame_data, frame_data_size).c_str());
    received_frame_type = frame_type;
    received_frame_size = frame_data_size;
    return true;
  }
};

bool test_api(bool print) {
  bool res = true;

  TestTionBleLtProtocol p;
  for (auto data : test_4s_data) {
    ApiTest t(&p);
    for (auto d : data.frames) {
      p.read_data(from_hex(d));
    }
    test_check(res, t.received_frame_type, data.await_frame_type);
    test_check(res, t.received_frame_size, data.await_frame_size);
  }

  return res;
}
