#pragma once

#include "esphome/components/uart/uart_component.h"

#include "../vport/vport.h"

namespace esphome {
namespace vport {

#define VPORT_UART_LOG(TAG, port_name) VPORT_LOG(TAG, port_name);

template<typename frame_type> class VPortUARTComponent : public VPortComponent<frame_type> {
 public:
  explicit VPortUARTComponent(uart::UARTComponent *uart) : uart_(uart) {}

 protected:
  uart::UARTComponent *uart_;
};

}  // namespace vport
}  // namespace esphome
