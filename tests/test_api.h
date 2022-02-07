#pragma once

#include <stdint.h>
#include <vector>
#include <string>

bool test_api(bool print);

struct ApiTestData {
  std::vector<std::string> frames;
  uint16_t await_frame_type = {};
  uint16_t await_frame_size = {};
  uint16_t await_struct = {};
};

enum { STATE = 1, DEV_STATUS, TURBO, TIME };
