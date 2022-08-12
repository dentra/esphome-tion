#pragma once

#include <cstddef>
#include <vector>
#include <string>
#include "../components/tion-api/tion-api-ble-lt.h"
bool test_api(bool print);

struct ApiTestData {
  std::vector<std::string> frames;
  uint16_t await_frame_type = {};
  uint16_t await_frame_size = {};
  uint16_t await_struct = {};
};

enum { STATE = 1, DEV_STATUS, TURBO, TIME };

class BleProtocolTest : public dentra::tion::TionBleLtProtocol {
 public:
  bool write_data(const uint8_t *data, size_t size) const override;
  bool read_data(const std::vector<uint8_t> &data) {
    return dentra::tion::TionBleLtProtocol::read_data(data.data(), data.size());
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
