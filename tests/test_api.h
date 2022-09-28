#pragma once

#include <cstddef>
#include <vector>
#include <string>
#include "../components/tion-api/tion-api-ble-lt.h"
#include "../components/tion-api/tion-api-ble-3s.h"
bool test_api(bool print);

struct ApiTestData {
  std::vector<std::string> frames;
  uint16_t await_frame_type = {};
  uint16_t await_frame_size = {};
  uint16_t await_struct = {};
};

enum { STATE = 1, DEV_STATUS, TURBO, TIME };

class TestTionBleLtProtocol : public dentra::tion::TionBleLtProtocol {
 public:
  bool read_data(const std::vector<uint8_t> &data) {
    return dentra::tion::TionBleLtProtocol::read_data(data.data(), data.size());
  }
};

class TestTionBle3sProtocol : public dentra::tion::TionBle3sProtocol {
 public:
  bool read_data(const std::vector<uint8_t> &data) {
    return dentra::tion::TionBle3sProtocol::read_data(data.data(), data.size());
  }
};
