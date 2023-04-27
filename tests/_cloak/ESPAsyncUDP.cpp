
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "ESPAsyncUDP.h"

static const char *const TAG = "cloak_async_udp";

bool AsyncUDP::listen(uint16_t port) {
  ip_addr_t addr{};
  return this->connect(&addr, port);
}

bool AsyncUDP::connect(const ip_addr_t *addr, uint16_t port) {
  ESP_LOGV(TAG, "Connected to %u.%u.%u.%u:%u", addr->addr >> 24 & 0xFF, addr->addr >> 16 & 0xFF, addr->addr >> 8 & 0xFF,
           addr->addr >> 0 & 0xFF, port);
  this->connected_ = true;
  this->remote_ip_ = *addr;
  this->remote_port_ = port;
  return true;
}

size_t AsyncUDP::writeTo(const uint8_t *data, size_t len, const ip_addr_t *addr, uint16_t port) {
  ESP_LOGV(TAG, "WriteTo %u.%u.%u.%u:%u %s", addr->addr >> 24 & 0xFF, addr->addr >> 16 & 0xFF, addr->addr >> 8 & 0xFF,
           addr->addr >> 0 & 0xFF, port, esphome::format_hex_pretty(data, len).c_str());
  this->cloak_data_.insert(this->cloak_data_.end(), data, data + len);
  return len;
}

void AsyncUDP::test_data_push(const void *data, size_t len) {
  auto str = esphome::format_hex_pretty(static_cast<const uint8_t *>(data), len);
  ESP_LOGV(TAG, "Push: %s", str.c_str());
  AsyncUDPPacket packet(this->remote_ip_, this->remote_port_, static_cast<const uint8_t *>(data), len);
  this->cb_packet_(packet);
}
