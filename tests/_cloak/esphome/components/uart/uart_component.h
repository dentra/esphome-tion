#pragma once
#include <cstring>
#include <string>
#include <vector>

#include "esphome/core/helpers.h"
#include "../../../string_uart.h"

namespace esphome {
namespace uart {

enum UARTParityOptions {
  UART_CONFIG_PARITY_NONE,
  UART_CONFIG_PARITY_EVEN,
  UART_CONFIG_PARITY_ODD,
};

#ifdef USE_UART_DEBUGGER
enum UARTDirection {
  UART_DIRECTION_RX,
  UART_DIRECTION_TX,
  UART_DIRECTION_BOTH,
};
#endif

class UARTComponent : public cloak::Cloak {
 public:
  UARTComponent() : str_uart_("") {}
  UARTComponent(const char *data) : str_uart_(data) {}
  UARTComponent(const uint8_t *data, size_t size) : str_uart_(data, size) {}
  UARTComponent(const std::vector<uint8_t> &data) : str_uart_(data.data(), data.size()) {}

  void write_str(const char *str) {
    const auto *data = reinterpret_cast<const uint8_t *>(str);
    this->write_array(data, strlen(str));
  };

  int available() { return this->str_uart_.available(); }
  bool read_array(void *data, size_t size) { return this->str_uart_.read_array(data, size); }
  int read() { return this->str_uart_.read(); }
  bool read_byte(uint8_t *ch) { return this->str_uart_.read_byte(ch); }
  bool peek_byte(uint8_t *data) { return this->str_uart_.peek_byte(data); }

  void write_array(const uint8_t *data, size_t len) {
    this->cloak_data_.insert(this->cloak_data_.end(), data, data + len);
  }
  void write_array(const std::vector<uint8_t> &data) { this->write_array(&data[0], data.size()); }
  void write_byte(uint8_t data) { this->write_array(&data, 1); }
  void flush() {}

 protected:
  cloak::internal::StringUart str_uart_;
};

}  // namespace uart
}  // namespace esphome
