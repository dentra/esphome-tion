#pragma once

#include <type_traits>
#include "etl/delegate.h"

#include "../components/tion_3s/vport/tion_3s_vport.h"
#include "../components/tion_lt/vport/tion_lt_vport.h"
#include "../components/tion_4s/vport/tion_4s_vport.h"
#include "../components/tion_4s_uart/tion_4s_uart.h"
#include "../components/tion_3s_uart/tion_3s_uart.h"

#include "utils.h"

using Tion4sUartIOTest = esphome::tion::Tion4sUartIO;
using Tion3sUartIOTest = esphome::tion::Tion3sUartIO;

class TionLtBleIOTest : public esphome::tion::TionLtBleIO, public cloak::Cloak {
 public:
  explicit TionLtBleIOTest(esphome::ble_client::BLEClient *client) {
    this->set_ble_client_parent(client);
    using this_t = typename std::remove_pointer_t<decltype(this)>;
    this->protocol_.writer.template set<this_t, &this_t::write_>(*this);
  }

  void test_data_push(const uint8_t *data, size_t size) override { this->on_ble_data(data, size); }
  void test_data_push(const std::vector<uint8_t> &data) override { cloak::Cloak::test_data_push(data); }

 protected:
  bool write_(const uint8_t *data, size_t size) {
    printf("[TionLtBleIOTest] BLE TX: %s\n", hexencode_cstr(data, size));
    this->cloak_data_.insert(this->cloak_data_.end(), data, data + size);
    return true;
  }
};

using Tion4sBleIOTest = TionLtBleIOTest;

class Tion3sBleIOTest : public esphome::tion::Tion3sBleIO, public cloak::Cloak {
 public:
  explicit Tion3sBleIOTest(esphome::ble_client::BLEClient *client) {
    this->set_ble_client_parent(client);
    using this_t = typename std::remove_pointer_t<decltype(this)>;
    this->protocol_.writer.template set<this_t, &this_t::write_>(*this);
  }

  void test_data_push(const uint8_t *data, size_t size) override { this->on_ble_data(data, size); }
  void test_data_push(const std::vector<uint8_t> &data) override { cloak::Cloak::test_data_push(data); }
  void test_data_push(const std::string &hex_data) { this->test_data_push(cloak::from_hex(hex_data)); }

 protected:
  bool write_(const uint8_t *data, size_t size) {
    printf("[TionBle3sIO] BLE TX: %s\n", hexencode_cstr(data, size));
    this->cloak_data_.insert(this->cloak_data_.end(), data, data + size);
    return true;
  }
};
