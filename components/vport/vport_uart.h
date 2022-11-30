#pragma once
#ifdef USE_VPORT_UART
#include "esphome/components/uart/uart_component.h"

#include "vport_component.h"

namespace esphome {
namespace vport {

#define VPORT_UART_LOG(TAG, port_name) VPORT_LOG(TAG, port_name);

/// Requires following protocol class.
/// interface ProtocolClass {
///   using writer_type = etl::delegate<bool(const uint8_t *data, size_t size)>;
///   using reader_type = etl::delegate<bool(uint16_t type, const void *data, size_t size)>;
///  public:
///   reader_type reader{};
///   writer_type writer{};
///   bool write_frame(uint16_t type, const void *data, size_t size);
/// }
template<typename frame_type, class protocol_type> class VPortUARTComponent : public VPortComponent<frame_type> {
  using this_type = VPortUARTComponent<frame_type, protocol_type>;

 public:
  explicit VPortUARTComponent(uart::UARTComponent *uart, protocol_type *protocol) : uart_(uart), protocol_(protocol) {
    this->protocol_->reader.template set<this_type, &this_type::read_frame>(*this);
    this->protocol_->writer.template set<this_type, &this_type::write_data>(*this);
  }

  void setup() override {
    this->defer([this] { this->fire_ready(); });
  }
  void update() override { this->fire_poll(); }

  // using writer_type = etl::delegate<bool(uint16_t type, const void *data, size_t size)>;
  // void setup_frame_writer(writer_type &writer) {
  //   writer.set<protocol_type, &protocol_type::write_frame>(*this->protocol_);
  // }

  bool read_frame(frame_type type, const void *data, size_t size) {
    this->fire_frame(type, data, size);
    return true;
  }

  bool write_data(const uint8_t *data, size_t size) {
    this->uart_->write_array(data, size);
    this->uart_->flush();
    return true;
  }

 protected:
  uart::UARTComponent *uart_;
  protocol_type *protocol_;
};

}  // namespace vport
}  // namespace esphome
#endif
