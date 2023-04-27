#include "utils.h"
#include "../components/tion-api/log.h"
#include "../components/tion-api/tion-api.h"
#include "../components/tion-api/tion-api-4s.h"
#include "test_api.h"

DEFINE_TAG;

using namespace dentra::tion;

class ApiTest {
 public:
  dentra::tion::TionApiBase<tion4s_state_t> api_;

  ApiTest(dentra::tion::TionBleLtProtocol *w) {
    w->reader.set<ApiTest, &ApiTest::read_frame>(*this);
    // w->writer.set<ApiTest, &ApiTest::write_data>(*this);
  }

  bool write_data(const uint8_t *data, size_t size) {
    ESP_LOGD(TAG, "Writing data: %s", hexencode_cstr(data, size));
    // wr_data_.insert(wr_data_.end(), data, data + size);
    return true;
  }

  uint16_t received_frame_type = {};
  uint16_t received_frame_size = {};

  void read_frame(const tion_any_ble_frame_t &frame, size_t frame_size) {
    ESP_LOGD(TAG, "Received frame data 0x%04X: %s", frame.type,
             hexencode_cstr(frame.data, frame_size - sizeof(frame.type)));
    received_frame_type = frame.type;
    received_frame_size = frame_size - frame.head_size();
  }
};

struct ApiTestData {
  enum { STATE = 1, DEV_STATUS, TURBO, TIME };
  std::vector<std::string> frames;
  uint16_t await_frame_type = {};
  uint16_t await_frame_size = {};
  uint16_t await_struct = {};
};

const ApiTestData test_4s_data[]{
    {
        .frames =
            {
                "00.2F.00.3A.D1.31.32.01.00.00.00.00.00.00.00.3C.51.00.10.01",
                "40.0C.17.12.1E.71.EF.29.00.D8.16.1F.00.28.37.CE.00.FE.56.43",
                "C0.00.00.00.00.00.06.00.63.1A",
            },
        .await_frame_type = 0x3231,
        .await_frame_size = 35,
        .await_struct = ApiTestData::STATE,

    },
    {
        .frames =
            {
                "00.25.00.3A.20.31.33.01.00.00.00.01.03.80.00.00.BC.02.01.00",
                "C0.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.05.23",
            },
        .await_frame_type = 0x3331,
        .await_frame_size = 25,
        .await_struct = ApiTestData::DEV_STATUS,
    },
};

bool test_api() {
  bool res = true;

  dentra::tion::TionBleLtProtocol p;
  for (auto data : test_4s_data) {
    ApiTest t(&p);
    for (auto d : data.frames) {
      auto x = cloak::from_hex(d);
      p.read_data(x.data(), x.size());
    }
    res &= cloak::check_data("frame_type", t.received_frame_type, data.await_frame_type);
    res &= cloak::check_data("frame_size", t.received_frame_size, data.await_frame_size);
  }

  return res;
}

REGISTER_TEST(test_api);
