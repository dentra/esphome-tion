#pragma once

#include "esphome/core/defines.h"
#include "esphome/components/uart/uart.h"

#include "../tion/tion.h"
#include "../tion/tion_vport_uart.h"
#include "../tion-api/tion-api-3s.h"
#include "../tion-api/tion-api-uart-3s.h"

namespace esphome {
namespace tion_3s_proxy {

class Tion3sUartProtocolProxy : public dentra::tion::Tion3sUartProtocol {
 public:
  Tion3sUartProtocolProxy() : dentra::tion::Tion3sUartProtocol(dentra::tion::FRAME_MAGIC_REQ) {}
};

class Tion3sUartIO : public tion::TionUartIO<Tion3sUartProtocolProxy> {
 public:
  explicit Tion3sUartIO(uart::UARTComponent *uart) : tion::TionUartIO<Tion3sUartProtocolProxy>(uart) {}
  virtual ~Tion3sUartIO() {}
  bool write_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
    return this->protocol_.write_frame(frame_type, frame_data, frame_data_size);
  }
};

class Tion3sProxy;

class Tion3sApiProxy : public dentra::tion::TionApiBase<dentra::tion::tion3s_state_t> {
 public:
  void read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size);
  void set_parent(Tion3sProxy *parent) { this->parent_ = parent; }

 protected:
  Tion3sProxy *parent_{};
};

class Tion3sProxy : public Component {
  friend class Tion3sApiProxy;

 public:
  explicit Tion3sProxy(Tion3sApiProxy *rx, uart::UARTComponent *uart) : rx_(rx) {
    this->tx_ = new Tion3sUartIO(uart);  // NOLINT cppcoreguidelines-owning-memory
    this->tx_->set_on_frame(Tion3sUartIO::on_frame_type::create<Tion3sProxy, &Tion3sProxy::on_frame_>(*this));
    this->rx_->set_parent(this);
  }
  virtual ~Tion3sProxy() { delete this->tx_; }
  void dump_config() override;
  void loop() override { this->tx_->poll(); }

 protected:
  void on_frame_(const Tion3sUartProtocolProxy::frame_spec_type &frame, size_t size);
  // tion
  Tion3sApiProxy *rx_{};
  // ble module
  Tion3sUartIO *tx_;

  uint8_t last_cmd_{};
};

}  // namespace tion_3s_proxy
}  // namespace esphome
