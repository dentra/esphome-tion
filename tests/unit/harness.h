#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "doctest/doctest.h"

#include "log.h"
#include "utils.h"
#include "tion-api-uart.h"

namespace test {

using bytes_t = std::vector<uint8_t>;

/// Parses hex string, any non-hex characters are ignored: "3A.07 00:32".
inline bytes_t from_hex(const std::string &hex) {
  bytes_t res;
  int hi = -1;
  for (const char c : hex) {
    int v;
    if (c >= '0' && c <= '9') {
      v = c - '0';
    } else if (c >= 'a' && c <= 'f') {
      v = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
      v = c - 'A' + 10;
    } else {
      continue;
    }
    if (hi < 0) {
      hi = v;
    } else {
      res.push_back(static_cast<uint8_t>((hi << 4) | v));
      hi = -1;
    }
  }
  REQUIRE_MESSAGE(hi < 0, "odd number of hex digits: ", hex);
  return res;
}

/// Formats bytes as "3A.07.00".
inline std::string to_hex(const bytes_t &data) {
  static const char hexmap[] = "0123456789ABCDEF";
  std::string res;
  for (size_t i = 0; i < data.size(); i++) {
    if (i > 0) {
      res += '.';
    }
    res += hexmap[data[i] >> 4];
    res += hexmap[data[i] & 0x0F];
  }
  return res;
}

/// Normalizes hex string to the to_hex() format to compare.
inline std::string hex(const std::string &hex) { return to_hex(from_hex(hex)); }

template<class T> bytes_t to_bytes(const T &data) {
  const auto *p = reinterpret_cast<const uint8_t *>(&data);
  return bytes_t(p, p + sizeof(T));
}

/// Controllable time source for tion-api millis().
class FakeClock {
 public:
  static uint32_t now() { return now_; }
  static void set(uint32_t ms) { now_ = ms; }
  static void advance(uint32_t ms) { now_ += ms; }

 private:
  static uint32_t now_;
};

/// UART input buffer for protocol readers.
class UartBuffer : public dentra::tion::TionUartReader {
 public:
  void push(const bytes_t &data) { this->data_.insert(this->data_.end(), data.begin(), data.end()); }
  int available() override { return static_cast<int>(this->data_.size() - this->pos_); }
  bool read_array(void *data, size_t size) override {
    if (size > static_cast<size_t>(this->available())) {
      return false;
    }
    std::memcpy(data, this->data_.data() + this->pos_, size);
    this->pos_ += size;
    return true;
  }

 protected:
  bytes_t data_;
  size_t pos_{};
};

/// Decoded frame: type and payload without protocol head.
struct Frame {
  uint16_t type;
  bytes_t data;

  template<class T> const T &as() const {
    REQUIRE_MESSAGE(this->data.size() == sizeof(T), "frame size ", this->data.size(), " != ", sizeof(T));
    return *reinterpret_cast<const T *>(this->data.data());
  }
};

/// Splits UART bytes into frames using a protocol instance.
template<class protocol_t, class... args_t> std::vector<Frame> decode_uart(const bytes_t &data, args_t... args) {
  using frame_spec_type = typename protocol_t::frame_spec_type;
  std::vector<Frame> frames;
  protocol_t protocol(args...);
  protocol.set_protocol_reader([&frames](const frame_spec_type &frame, size_t size) {
    const auto *p = reinterpret_cast<const uint8_t *>(frame.data);
    frames.push_back({frame.type, bytes_t(p, p + size - frame_spec_type::head_size())});
  });
  UartBuffer buf;
  buf.push(data);
  while (buf.available() > 0) {
    const auto before = buf.available();
    protocol.read_uart_data(&buf);
    if (buf.available() == before) {
      break;
    }
  }
  return frames;
}

/// Encodes a frame to UART bytes using a protocol instance.
template<class protocol_t, class T, class... args_t> bytes_t encode_uart(uint16_t type, const T &data, args_t... args) {
  bytes_t res;
  protocol_t protocol(args...);
  protocol.set_protocol_writer([&res](const uint8_t *p, size_t size) {
    res.insert(res.end(), p, p + size);
    return true;
  });
  protocol.write_frame(type, &data, sizeof(T));
  return res;
}

/// Encodes a frame to BLE packets using a protocol instance.
template<class protocol_t, class T> std::vector<bytes_t> encode_ble(uint16_t type, const T &data) {
  std::vector<bytes_t> res;
  protocol_t protocol;
  protocol.set_protocol_writer([&res](const uint8_t *p, size_t size) {
    res.emplace_back(p, p + size);
    return true;
  });
  protocol.write_frame(type, &data, sizeof(T));
  return res;
}

/// Connects api with a protocol: api -> protocol -> tx bytes, rx bytes -> protocol -> api.
template<class api_t, class protocol_t> class Harness {
 public:
  using frame_spec_type = typename protocol_t::frame_spec_type;

  api_t api;
  protocol_t protocol;
  /// Bytes written to the breezer.
  bytes_t tx;
  /// Number of frames passed to api.
  int rx_frames{};

  Harness() {
    this->api.set_api_writer([this](uint16_t type, const void *data, size_t size) {
      return this->protocol.write_frame(type, data, size);
    });
    this->protocol.set_protocol_writer([this](const uint8_t *data, size_t size) {
      this->tx.insert(this->tx.end(), data, data + size);
      return true;
    });
    this->protocol.set_protocol_reader([this](const frame_spec_type &frame, size_t size) {
      this->rx_frames++;
      this->api.read_frame(frame.type, frame.data, size - frame_spec_type::head_size());
    });
  }

  /// Returns written bytes as hex and clears them.
  std::string take_tx() {
    auto res = to_hex(this->tx);
    this->tx.clear();
    return res;
  }
};

/// Harness for UART protocols.
template<class api_t, class protocol_t> class UartHarness : public Harness<api_t, protocol_t> {
 public:
  UartBuffer rx;

  /// Feeds bytes received from the breezer.
  void receive(const std::string &hex) { this->receive(from_hex(hex)); }
  void receive(const bytes_t &data) {
    this->rx.push(data);
    while (this->rx.available() > 0) {
      const auto before = this->rx.available();
      this->protocol.read_uart_data(&this->rx);
      if (this->rx.available() == before) {
        break;
      }
    }
  }
};

/// Harness for BLE protocols.
template<class api_t, class protocol_t> class BleHarness : public Harness<api_t, protocol_t> {
 public:
  /// Feeds one BLE packet received from the breezer.
  bool receive(const std::string &hex) { return this->receive(from_hex(hex)); }
  bool receive(const bytes_t &data) { return this->protocol.read_data(data.data(), data.size()); }
  /// Feeds several BLE packets.
  void receive(const std::vector<bytes_t> &packets) {
    for (auto &&p : packets) {
      this->receive(p);
    }
  }
};

}  // namespace test
