#pragma once

#include "esphome/core/defines.h"
#include "esphome/components/uart/uart.h"

#include "../tion/tion.h"
#include "../tion/tion_vport_uart.h"
#include "../tion-api/tion-api-3s.h"
#include "../tion-api/tion-api-uart-3s.h"

namespace esphome {
namespace tion_3s_proxy {

class TionUartProtocol3sProxy : public dentra::tion::TionUartProtocol3s {
 public:
  TionUartProtocol3sProxy() { this->head_type_ = dentra::tion::FRAME_MAGIC_REQ; }
};

class Tion3sUartIO : public tion::TionUartIO<TionUartProtocol3sProxy> {
 public:
  explicit Tion3sUartIO(uart::UARTComponent *uart) : tion::TionUartIO<TionUartProtocol3sProxy>(uart) {}
  virtual ~Tion3sUartIO() {}
  bool write_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
    return this->protocol_.write_frame(frame_type, frame_data, frame_data_size);
  }
};

class TionApi3sProxy : public dentra::tion::TionApiBase<dentra::tion::tion3s_state_t> {
 public:
  void read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size);
  void set_tx(Tion3sUartIO *tx) { this->tx_ = tx; }

 protected:
  Tion3sUartIO *tx_{};
};

class Tion3sProxy : public Component {
 public:
  explicit Tion3sProxy(TionApi3sProxy *rx, uart::UARTComponent *uart) : rx_(rx) {
    this->tx_ = new Tion3sUartIO(uart);
    this->tx_->set_on_frame(Tion3sUartIO::on_frame_type::create<Tion3sProxy, &Tion3sProxy::on_frame_>(*this));
    this->rx_->set_tx(this->tx_);
  }
  virtual ~Tion3sProxy() { delete this->tx_; }
  void dump_config() override;
  void loop() { this->tx_->poll(); }

 protected:
  void on_frame_(const TionUartProtocol3sProxy::frame_spec_type &frame, size_t size);
  // tion
  TionApi3sProxy *rx_{};
  // ble module
  Tion3sUartIO *tx_;
};

}  // namespace tion_3s_proxy
}  // namespace esphome
