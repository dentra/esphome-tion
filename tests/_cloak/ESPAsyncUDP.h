#pragma once

#include <cstdio>
#include <vector>
#include <functional>

#include "cloak.h"

struct ip_addr_t {
  uint32_t addr;
};

inline void ipaddr_aton(const char *addr_in, ip_addr_t *addr_out) { addr_out->addr = 0; }

struct IPAddress {
  IPAddress(ip_addr_t addr) : ip(addr) {}
  ip_addr_t ip{};
  std::string toString() { return "[not-implemented-addr-to-string]"; }
  operator uint32_t() const { return ip.addr; }
};

class AsyncUDPPacket {
 public:
  AsyncUDPPacket(const ip_addr_t &remote_ip, uint16_t remote_port, const uint8_t *data, size_t len)
      : remote_ip_(remote_ip), remote_port_(remote_port), data_(data), len_(len) {}
  const uint8_t *data() const { return this->data_; }
  size_t length() const { return this->len_; }
  const ip_addr_t *remoteIP() const { return &this->remote_ip_; }
  uint16_t remotePort() const { return this->remote_port_; }

 protected:
  ip_addr_t remote_ip_{};
  uint16_t remote_port_{};
  const uint8_t *data_;
  size_t len_;
};

typedef std::function<void(AsyncUDPPacket &packet)> AuPacketHandlerFunction;

class AsyncUDP : public cloak::Cloak {
 public:
  bool connect(const ip_addr_t *addr, uint16_t port);
  bool listen(uint16_t port);
  bool listenMulticast(const ip_addr_t *addr, uint16_t port);
  void onPacket(AuPacketHandlerFunction &&cb) { this->cb_packet_ = std::move(cb); }
  size_t writeTo(const uint8_t *data, size_t len, const ip_addr_t *addr, uint16_t port);

  void test_data_push(const void *data, size_t len);
  void test_data_push(const char *str) { this->test_data_push(str, strlen(str)); }
  void test_data_push(std::vector<uint8_t> data) { this->test_data_push(data.data(), data.size()); }

  void close() { this->connected_ = false; }
  bool connected() const { return this->connected_; }

 protected:
  bool connected_{};
  ip_addr_t remote_ip_{};
  uint16_t remote_port_{};
  AuPacketHandlerFunction cb_packet_;
};
