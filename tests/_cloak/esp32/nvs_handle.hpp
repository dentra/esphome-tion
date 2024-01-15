#pragma once
#include <memory>
#include <type_traits>
#include "nvs_flash.h"

namespace nvs {

/**
 * The possible blob types. This is a helper definition for template functions.
 */
enum class ItemType : uint8_t {
  U8 = NVS_TYPE_U8,
  I8 = NVS_TYPE_I8,
  U16 = NVS_TYPE_U16,
  I16 = NVS_TYPE_I16,
  U32 = NVS_TYPE_U32,
  I32 = NVS_TYPE_I32,
  U64 = NVS_TYPE_U64,
  I64 = NVS_TYPE_I64,
  SZ = NVS_TYPE_STR,
  BLOB = 0x41,
  BLOB_DATA = NVS_TYPE_BLOB,
  BLOB_IDX = 0x48,
  ANY = NVS_TYPE_ANY
};

class NVSHandle {
 public:
  NVSHandle(nvs_handle_t h) : h_(h) {}
  template<typename T> esp_err_t set_item(const char *key, T value) {
    return internal::nvs_set(this->h_, key, &value, sizeof(value));
  }
  template<typename T> esp_err_t get_item(const char *key, T &value) {
    size_t size = sizeof(value);
    return internal::nvs_get(this->h_, key, &value, &size);
  }
  esp_err_t set_string(const char *key, const char *value) { return nvs_set_str(this->h_, key, value); }
  esp_err_t set_blob(const char *key, const void *blob, size_t len) { return nvs_set_blob(this->h_, key, blob, len); }
  esp_err_t get_string(const char *key, char *out_str, size_t len) { return nvs_get_str(this->h_, key, out_str, &len); }
  esp_err_t get_blob(const char *key, void *out_blob, size_t len) {
    return nvs_get_blob(this->h_, key, out_blob, &len);
  }
  esp_err_t get_item_size(ItemType datatype, const char *key, size_t &size) {
    return internal::nvs_get(this->h_, key, nullptr, &size);
  }
  esp_err_t erase_item(const char *key) { return nvs_erase_key(this->h_, key); }
  esp_err_t commit() { return nvs_commit(this->h_); }

 protected:
  nvs_handle_t h_;
};

inline std::unique_ptr<NVSHandle> open_nvs_handle(const char *ns_name, nvs_open_mode_t open_mode,
                                                  esp_err_t *err = nullptr) {
  nvs_handle_t h{};
  auto res = nvs_open(ns_name, open_mode, &h);
  if (err) {
    *err = res;
  }
  if (res != ESP_OK) {
    return {};
  }
  return std::make_unique<NVSHandle>(h);
}

}  // namespace nvs
