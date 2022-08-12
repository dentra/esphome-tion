#pragma once

#include "../tion/tion.h"
#include "../vport_uart/vport_uart.h"
#include "../tion-api/tion-api-uart.h"

namespace esphome {
namespace tion {

class TionUARTVPort final : public vport::VPortUARTComponent<uint16_t>,
                            public dentra::tion::TionUartReader,
                            public TionVPort {
  friend class VPortTionUartProtocol;

 public:
  explicit TionUARTVPort(uart::UARTComponent *uart) : VPortUARTComponent(uart) {}

  void dump_config() override;
  void setup() override;
  void update() override;
  void loop() override { this->protocol_->read_uart_data(this); }

  void set_heartbeat_interval(uint32_t heartbeat_interval) { this->heartbeat_interval_ = heartbeat_interval; }

  // TionUartReader implementation
  int available() override { return this->uart_->available(); };
  bool peek_byte(uint8_t *data) override { return this->uart_->peek_byte(data); }
  bool read_array(void *data, size_t size) override {
    return this->uart_->read_array(reinterpret_cast<uint8_t *>(data), size);
  }

  void set_state_type(uint16_t state_type) {}

  void set_protocol(dentra::tion::TionUartProtocol *protocol) { this->protocol_ = protocol; }

  // TionVport implementation
  dentra::tion::TionProtocol *get_protocol() const override { return this->protocol_; }
  void schedule_poll() override { this->update(); }
  TionVPort::Type get_vport_type() const override { return TionVPort::Type::VPORT_UART; }

 protected:
  uint32_t heartbeat_interval_{};
  dentra::tion::TionUartProtocol *protocol_{};
};

class VPortTionUartProtocol : public dentra::tion::TionUartProtocol, public Parented<TionUARTVPort> {
 public:
  VPortTionUartProtocol(TionUARTVPort *vport) : Parented(vport) { vport->set_protocol(this); }
  bool read_frame(uint16_t type, const void *data, size_t size) override;
  bool write_data(const uint8_t *data, size_t size) const override;
};

}  // namespace tion
}  // namespace esphome
