
#include "utils.h"
#include "../components/tion-api/log.h"
#include "../components/tion-api/tion-api-lt.h"
#include "test_api.h"
using namespace dentra::tion;

static std::vector<uint8_t> wr_data_;

class BleProtocolLtTest : public BleProtocolTest {
 public:
  bool write_data(const uint8_t *data, size_t size) const override {
    LOGD("Writting data: %s", hexencode(data, size).c_str());
    wr_data_.insert(wr_data_.end(), data, data + size);
    return true;
  }
};

class ApiLtTest : public TionApiLt {
 public:
  explicit ApiLtTest(BleProtocolLtTest *writer) : TionApiLt(writer) {}
};

bool test_api_lt(bool print) {
  bool res = true;
  BleProtocolLtTest p;
  ApiLtTest tlt(&p);
  p.set_api(&tlt);

  wr_data_.clear();
  tlt.request_state();
  test_check(res, wr_data_, from_hex("80.0C.00.3A.AD.32.12.01.00.00.00.6C.43"));

  wr_data_.clear();
  tlt.request_dev_status();
  test_check(res, wr_data_, from_hex("80.0C.00.3A.AD.09.40.01.00.00.00.D1.DC"));

  tionlt_state_t st{};
  st.flags.power_state = 0;
  st.counters.work_time = 1;

  wr_data_.clear();
  tlt.write_state(st, 1);
  test_check(
      res, wr_data_,
      from_hex("00.1E.00.3A.AD.30.12.01.00.00.00.01.00.00.00.00.00.00.00.00.C0.00.00.00.00.00.00.00.00.00.F8.B7"));

  return res;
}
