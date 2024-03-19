#pragma once

#include "esphome/core/defines.h"
#include "esphome/components/uart/uart.h"

#include "../tion/tion_vport_uart.h"
#include "../tion-api/tion-api-3s.h"
#include "../tion-api/tion-api-uart-3s.h"

namespace esphome {
namespace tion_3s_proxy {

class Tion3sBleProxy;

// This component is connected directly to tion breezer.
class Tion3sApiProxy : public dentra::tion::TionApiBase {
 public:
  using Api = Tion3sApiProxy;  // used in TionVPortApi wrapper

  void read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size);
  void set_ble(Tion3sBleProxy *ble) { this->ble_ = ble; }

 protected:
  Tion3sBleProxy *ble_{};
  // not used here
  void request_state() override {}
  void write_state(dentra::tion::TionStateCall *call) override {}
  void reset_filter() override {}
};

class Tion3sUartProtocolProxy : public dentra::tion::Tion3sUartProtocol {
 public:
  Tion3sUartProtocolProxy() : dentra::tion::Tion3sUartProtocol(dentra::tion_3s::FRAME_MAGIC_REQ) {}
};

// This component is connected directly to tion ble module.
// *api - tion breezer
// *uart - tion ble module
class Tion3sBleProxy : public Component, public tion::TionUartIO<Tion3sUartProtocolProxy> {
 public:
  explicit Tion3sBleProxy(Tion3sApiProxy *api, uart::UARTComponent *uart)
      : tion::TionUartIO<Tion3sUartProtocolProxy>(uart), api_(api) {
    this->set_on_frame(on_frame_type::create<Tion3sBleProxy, &Tion3sBleProxy::on_frame_>(*this));
    this->api_->set_ble(this);
  }

  void dump_config() override;
  void loop() override { this->poll(); }


  void write_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
    this->protocol_.write_frame(frame_type, frame_data, frame_data_size);
  }

  uint8_t get_last_cmd() const { return this->last_cmd_; }
  void reset_last_cmd() {this->last_cmd_ = 0;}

 protected:
  Tion3sApiProxy *api_;
  uint8_t last_cmd_{};

  void on_frame_(const frame_spec_type &frame, size_t size);
};

}  // namespace tion_3s_proxy
}  // namespace esphome
