#pragma once

#include <cstddef>
#include <vector>
#include <string>
#include "../components/tion-api/tion-api-ble-lt.h"
#include "../components/tion-api/tion-api-ble-3s.h"

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

template<class api_t> class TionComponentTest {
 public:
  using api_type = api_t;
  TionComponentTest(api_t *api) : api_(api) {
    using this_t = typename std::remove_pointer_t<decltype(this)>;
    api->set_on_ready(api_t::on_ready_type::template create<this_t, &this_t::ready_>(*this));
  }

  bool is_ready() { return this->is_ready_; }

 protected:
  api_t *api_;
  bool is_ready_{};
  void ready_() { this->is_ready_ = true; }
};
