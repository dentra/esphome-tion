#pragma once

#include "../tion/tion.h"
#include "../vport/vport_uart.h"
#include "../tion-api/tion-api-uart.h"

namespace esphome {
namespace tion {

using VPortTionUartProtocol = dentra::tion::TionUartProtocol;

using TionUARTVPortBase = vport::VPortUARTComponent<uint16_t, VPortTionUartProtocol>;

class TionUARTVPort final : public TionUARTVPortBase, public dentra::tion::TionUartReader {
 public:
  explicit TionUARTVPort(uart::UARTComponent *uart, VPortTionUartProtocol *protocol)
      : TionUARTVPortBase(uart, protocol) {}

  void dump_config() override;
  void setup() override;
  void loop() override { this->protocol_->read_uart_data(this); }

  void set_heartbeat_interval(uint32_t heartbeat_interval) { this->heartbeat_interval_ = heartbeat_interval; }

  // TionUartReader implementation
  int available() override { return this->uart_->available(); };
  bool read_array(void *data, size_t size) override {
    return this->uart_->read_array(reinterpret_cast<uint8_t *>(data), size);
  }

  void set_state_type(uint16_t state_type) {}

  TionVPortType get_vport_type() const { return TionVPortType::VPORT_UART; }

  void set_cc(TionClimateComponentBase *cc) { this->cc_ = cc; }

  // TODO update in py
  void register_api_writer(dentra::tion::TionApiBaseWriter *api) {
    // this->setup_frame_writer(api->writer);
    api->writer.set<VPortTionUartProtocol, &VPortTionUartProtocol::write_frame>(*this->protocol_);
  }

 protected:
  uint32_t heartbeat_interval_{};
  TionClimateComponentBase *cc_{};
};

}  // namespace tion
}  // namespace esphome
