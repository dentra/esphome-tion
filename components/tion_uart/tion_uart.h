#pragma once

#include "../tion/tion.h"
#include "../vport_uart/vport_uart.h"
#include "../tion-api/tion-api-uart.h"

namespace esphome {
namespace tion {

class TionUARTVPort final : public vport::VPortUARTComponent<uint16_t>, public dentra::tion::TionUartReader {
 public:
  explicit TionUARTVPort(uart::UARTComponent *uart, dentra::tion::TionUartProtocol *protocol)
      : VPortUARTComponent(uart), protocol_(protocol) {
    this->protocol_->reader.set<TionUARTVPort, &TionUARTVPort::read_frame>(*this);
    this->protocol_->writer.set<TionUARTVPort, &TionUARTVPort::write_data>(*this);
  }

  void dump_config() override;
  void setup() override;
  void update() override;
  void loop() override { this->protocol_->read_uart_data(this); }

  void set_heartbeat_interval(uint32_t heartbeat_interval) { this->heartbeat_interval_ = heartbeat_interval; }

  // TionUartReader implementation
  int available() override { return this->uart_->available(); };
  bool read_array(void *data, size_t size) override {
    return this->uart_->read_array(reinterpret_cast<uint8_t *>(data), size);
  }

  void set_state_type(uint16_t state_type) {}

  TionVPortType get_vport_type() const { return TionVPortType::VPORT_UART; }

  bool read_frame(uint16_t type, const void *data, size_t size);
  bool write_data(const uint8_t *data, size_t size);

  void set_cc(TionClimateComponentBase *cc) { this->cc_ = cc; }

  bool write_frame(uint16_t type, const void *data, size_t size) {
    return this->protocol_->write_frame(type, data, size);
  }

  void register_api_writer(dentra::tion::TionApiBaseWriter *api) {
    api->writer.set<TionUARTVPort, &TionUARTVPort::write_frame>(*this);
    // api->writer.set<dentra::tion::TionUartProtocol, &dentra::tion::TionUartProtocol::write_frame>(*this->protocol_);
  }

 protected:
  dentra::tion::TionUartProtocol *protocol_;
  uint32_t heartbeat_interval_{};
  TionClimateComponentBase *cc_{};
};

class VPortTionUartProtocol : public dentra::tion::TionUartProtocol {};

}  // namespace tion
}  // namespace esphome
