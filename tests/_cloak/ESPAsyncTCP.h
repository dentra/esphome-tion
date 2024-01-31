#pragma once

#include <cstdint>
#include <vector>
#include <functional>

#include "cloak.h"

class AsyncClient;

typedef std::function<void(void *, AsyncClient *)> AcConnectHandler;
typedef std::function<void(void *, AsyncClient *, void *data, size_t len)> AcDataHandler;

class AsyncClient : public cloak::Cloak {
 public:
  ~AsyncClient() { this->disconnect(); }
  bool connect(const char *host, uint16_t port);
  void disconnect();
  size_t add(const char *data, size_t size, int flags = 0) {
    this->data_.insert(this->data_.end(), data, data + size);
    return size;
  }
  bool send();

  void onConnect(AcConnectHandler &&cb, void *arg = nullptr) {
    this->cb_connect_ = std::move(cb);
    this->arg_connect_ = arg;
  }
  void onDisconnect(AcConnectHandler &&cb, void *arg = nullptr) {
    this->cb_disconnect_ = std::move(cb);
    this->arg_disconnect_ = arg;
  }
  void onData(AcDataHandler &&cb, void *arg = nullptr) {
    this->cb_data_ = std::move(cb);
    this->arg_data_ = arg;
  }
  bool connected() { return this->connected_; }
  void close() { this->disconnect(); }

  void test_data_push(const void *data, size_t len);
  void test_data_push(const char *str) { this->test_data_push(str, strlen(str)); }
  void test_data_push(std::vector<uint8_t> data) { this->test_data_push(data.data(), data.size()); }

 protected:
  std::vector<char> data_;
  bool connected_{};
  AcConnectHandler cb_connect_;
  void *arg_connect_{};
  AcConnectHandler cb_disconnect_;
  void *arg_disconnect_{};
  AcDataHandler cb_data_;
  void *arg_data_{};
};
