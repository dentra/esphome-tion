#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <memory>

#include "nvs_flash.h"

const char *esp_err_to_name(esp_err_t err) {
  if (err == ESP_OK) {
    return "OK";
  }
  return "ERROR";
}

struct ns_container_t {
  std::string namespace_name;
  std::map<std::string, std::vector<uint8_t>> items;
};

static std::map<std::string, ns_container_t *> _namespaces;

ns_container_t *_cast(nvs_handle_t handle) { return static_cast<ns_container_t *>(handle); }

esp_err_t nvs_flash_init() { return ESP_OK; }

esp_err_t nvs_flash_deinit() { return ESP_OK; }

esp_err_t nvs_flash_erase() {
  for (auto it : _namespaces) {
    delete it.second;
  }
  _namespaces.clear();
  return ESP_OK;
}

esp_err_t nvs_open(const char *namespace_name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle) {
  auto it = _namespaces.find(namespace_name);
  if (it == _namespaces.end()) {
    auto *c = new ns_container_t;
    c->namespace_name = namespace_name;
    _namespaces.emplace(namespace_name, c);
    it = _namespaces.find(namespace_name);
  }
  *out_handle = static_cast<nvs_handle_t>(it->second);
  return ESP_OK;
}

esp_err_t nvs_commit(nvs_handle_t handle) { return ESP_OK; }

esp_err_t nvs_close(nvs_handle_t handle) {
  if (handle == nullptr) {
    return ESP_FAIL;
  }

  auto it = _namespaces.find(_cast(handle)->namespace_name);
  if (it == _namespaces.end()) {
    return ESP_FAIL;
  }

  delete it->second;
  _namespaces.erase(it);

  return ESP_OK;
}

esp_err_t nvs_erase_key(nvs_handle_t c_handle, const char *key) {
  auto c = _cast(c_handle);
  if (c == nullptr) {
    return ESP_FAIL;
  }
  c->items.erase(key);
  return ESP_OK;
}

namespace nvs {
namespace internal {

esp_err_t nvs_set(nvs_handle_t handle, const char *key, const void *value, size_t value_size) {
  auto c = _cast(handle);
  if (c == nullptr) {
    return ESP_FAIL;
  }
  std::vector<uint8_t> v;
  v.resize(value_size);
  std::memcpy(v.data(), value, value_size);
  c->items.emplace(key, v);
  return ESP_OK;
}

esp_err_t nvs_get(nvs_handle_t handle, const char *key, void *value, size_t *value_size) {
  auto c = _cast(handle);
  if (c == nullptr || value_size == nullptr) {
    return ESP_FAIL;
  }
  auto it = c->items.find(key);
  if (it == c->items.end()) {
    return ESP_FAIL;
  }
  if (value == nullptr) {
    *value_size = it->second.size();
    return ESP_OK;
  }
  if (it->second.size() != *value_size) {
    return ESP_FAIL;
  }
  std::memcpy(value, it->second.data(), *value_size);
  return ESP_OK;
}

}  // namespace internal
}  // namespace nvs

using namespace nvs::internal;

template<typename T> inline esp_err_t nvs_set(nvs_handle_t handle, const char *key, T value) {
  return nvs_set(handle, key, &value, sizeof(value));
}

template<typename T> inline esp_err_t nvs_get(nvs_handle_t handle, const char *key, T *value) {
  size_t size = sizeof(*value);
  return nvs_get(handle, key, value, &size);
}

esp_err_t nvs_get_i8(nvs_handle_t c_handle, const char *key, int8_t *out_value) {
  return nvs_get(c_handle, key, out_value);
}

esp_err_t nvs_get_u8(nvs_handle_t c_handle, const char *key, uint8_t *out_value) {
  return nvs_get(c_handle, key, out_value);
}

esp_err_t nvs_get_i16(nvs_handle_t c_handle, const char *key, int16_t *out_value) {
  return nvs_get(c_handle, key, out_value);
}

esp_err_t nvs_get_u16(nvs_handle_t c_handle, const char *key, uint16_t *out_value) {
  return nvs_get(c_handle, key, out_value);
}

esp_err_t nvs_get_i32(nvs_handle_t c_handle, const char *key, int32_t *out_value) {
  return nvs_get(c_handle, key, out_value);
}

esp_err_t nvs_get_u32(nvs_handle_t c_handle, const char *key, uint32_t *out_value) {
  return nvs_get(c_handle, key, out_value);
}

esp_err_t nvs_get_i64(nvs_handle_t c_handle, const char *key, int64_t *out_value) {
  return nvs_get(c_handle, key, out_value);
}

esp_err_t nvs_get_u64(nvs_handle_t c_handle, const char *key, uint64_t *out_value) {
  return nvs_get(c_handle, key, out_value);
}

esp_err_t nvs_get_str(nvs_handle_t c_handle, const char *key, char *out_value, size_t *length) {
  auto res = nvs_get(c_handle, key, out_value, length);
  if (out_value != nullptr && res == ESP_OK) {
    *length = (*length) - 1;
  }
  return res;
}

esp_err_t nvs_get_blob(nvs_handle_t c_handle, const char *key, void *out_value, size_t *length) {
  return nvs_get(c_handle, key, out_value, length);
}

esp_err_t nvs_set_i8(nvs_handle_t handle, const char *key, int8_t value) { return nvs_set(handle, key, value); }

esp_err_t nvs_set_u8(nvs_handle_t handle, const char *key, uint8_t value) { return nvs_set(handle, key, value); }

esp_err_t nvs_set_i16(nvs_handle_t handle, const char *key, int16_t value) { return nvs_set(handle, key, value); }

esp_err_t nvs_set_u16(nvs_handle_t handle, const char *key, uint16_t value) { return nvs_set(handle, key, value); }

esp_err_t nvs_set_i32(nvs_handle_t handle, const char *key, int32_t value) { return nvs_set(handle, key, value); }

esp_err_t nvs_set_u32(nvs_handle_t handle, const char *key, uint32_t value) { return nvs_set(handle, key, value); }

esp_err_t nvs_set_i64(nvs_handle_t handle, const char *key, int64_t value) { return nvs_set(handle, key, value); }

esp_err_t nvs_set_u64(nvs_handle_t handle, const char *key, uint64_t value) { return nvs_set(handle, key, value); }

esp_err_t nvs_set_str(nvs_handle_t c_handle, const char *key, const char *value) {
  return nvs_set(c_handle, key, value, strlen(value) + 1);
}

esp_err_t nvs_set_blob(nvs_handle_t c_handle, const char *key, const void *value, size_t length) {
  return nvs_set(c_handle, key, value, length);
}
