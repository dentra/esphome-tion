#pragma once

#include "esphome/components/uart/uart_component.h"

#include "../vport/vport.h"

namespace esphome {
namespace vport {

template<typename T> class VPortUARTComponent : public VPortComponent<T> {
 public:
  explicit VPortUARTComponent(uart::UARTComponent *uart) : uart_(uart) {}

 protected:
  uart::UARTComponent *uart_;
};

}  // namespace vport
}  // namespace esphome
