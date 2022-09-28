#pragma once
#include "../components/vport/vport.h"
#include "../components/tion-api/tion-api-ble-lt.h"
#include "../components/tion-api/tion-api-uart.h"
using namespace esphome::vport;
using namespace dentra::tion;
#include "utils.h"

#include "etl/delegate.h"

template<typename frame_type> class VPortComponent : public VPort<frame_type> {
 public:
  virtual void loop() = 0;
};

template<class protocol_type> class TionVPortComponent : public VPortComponent<uint16_t> {
  static_assert(std::is_base_of<dentra::tion::TionProtocol, protocol_type>::value,
                "ProtocolT must derived from dentra::tion::TionProtocol class");

  using this_type = TionVPortComponent<protocol_type>;

 public:
  explicit TionVPortComponent(protocol_type *protocol) : protocol_(protocol) {
    protocol->reader.template set<this_type, &this_type::read_frame>(*this);
    protocol->writer.template set<this_type, &this_type::write_data>(*this);
  }

  bool read_frame(uint16_t type, const void *data, size_t size) {
    this->fire_frame(type, data, size);
    return true;
  }

  bool write_data(const uint8_t *data, size_t size) {
    printf("[TionVPortComponent] write_data: %s\n", hexencode(data, size).c_str());
    return true;
  }

 protected:
  protocol_type *protocol_;
};

class StringBleVPort : public TionVPortComponent<TionBleLtProtocol> {
 public:
  StringBleVPort(const char *data, TionBleLtProtocol *protocol) : TionVPortComponent(protocol), data_(data) {}
  void loop() override {
    auto data = from_hex(this->data_);
    this->protocol_->read_data(data.data(), data.size());
  }

 protected:
  const char *data_;
};
