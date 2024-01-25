#pragma once

#include "esphome/core/defines.h"
#include "esphome/components/uart/uart.h"

#include "../tion/tion.h"
#include "../tion/tion_vport_uart.h"
#include "../tion-api/tion-api-o2.h"
#include "../tion-api/tion-api-uart-o2.h"

namespace esphome {
namespace tion_o2_proxy {

class TionO2UartProtocolProxy : public dentra::tion_o2::TionO2UartProtocol {
 public:
  TionO2UartProtocolProxy() : dentra::tion_o2::TionO2UartProtocol(true) {}
};

class TionO2UartIO : public tion::TionUartIO<TionO2UartProtocolProxy> {
 public:
  explicit TionO2UartIO(uart::UARTComponent *uart) : tion::TionUartIO<TionO2UartProtocolProxy>(uart) {}
  virtual ~TionO2UartIO() {}
  bool write_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
    return this->protocol_.write_frame(frame_type, frame_data, frame_data_size);
  }
};

class TionO2Proxy;

class TionO2ApiProxy : public dentra::tion::TionApiBase<dentra::tion_o2::tiono2_state_t> {
 public:
  void read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size);
  void set_parent(TionO2Proxy *parent) { this->parent_ = parent; }

 protected:
  TionO2Proxy *parent_{};
};

class TionO2Proxy : public Component {
  friend class TionO2ApiProxy;

 public:
  explicit TionO2Proxy(TionO2ApiProxy *rx, uart::UARTComponent *uart) : rx_(rx) {
    this->tx_ = new TionO2UartIO(uart);  // NOLINT cppcoreguidelines-owning-memory
    this->tx_->set_on_frame(TionO2UartIO::on_frame_type::create<TionO2Proxy, &TionO2Proxy::on_frame_>(*this));
    this->rx_->set_parent(this);
  }
  virtual ~TionO2Proxy() { delete this->tx_; }
  void dump_config() override;
  void loop() override { this->tx_->poll(); }

 protected:
  void on_frame_(const TionO2UartProtocolProxy::frame_spec_type &frame, size_t size);
  // tion
  TionO2ApiProxy *rx_{};
  // RF module
  TionO2UartIO *tx_;
};

}  // namespace tion_o2_proxy
}  // namespace esphome
