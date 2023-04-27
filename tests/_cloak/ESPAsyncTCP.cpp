#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "ESPAsyncTCP.h"

static const char *const TAG = "cloak_async_tcp";

bool AsyncClient::connect(const char *host, uint16_t port) {
  ESP_LOGV(TAG, "Connected to %s:%u", host, port);
  this->connected_ = true;
  this->cb_connect_(this->arg_connect_, this);
  return true;
}

void AsyncClient::disconnect() {
  ESP_LOGV(TAG, "Disconnected");
  this->connected_ = false;
  this->cb_disconnect_(this->arg_disconnect_, this);
}

bool AsyncClient::send() {
  auto str = esphome::format_hex_pretty(reinterpret_cast<const uint8_t *>(this->data_.data()), this->data_.size());
  ESP_LOGV(TAG, "Send: %s", str.c_str());
  this->cloak_data_.insert(this->cloak_data_.end(), this->data_.begin(), this->data_.end());
  this->data_.clear();
  return true;
}

void AsyncClient::test_data_push(const void *data, size_t len) {
  auto str = esphome::format_hex_pretty(static_cast<const uint8_t *>(data), len);
  ESP_LOGV(TAG, "Push: %s", str.c_str());
  this->cb_data_(this->arg_data_, this, const_cast<void *>(data), len);
}
