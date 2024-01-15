#pragma once

#include <cstdint>
#include <cstddef>

#ifndef esp_err_t
#define esp_err_t int
#endif
#ifndef ESP_OK
#define ESP_OK 0
#endif
#ifndef ESP_FAIL
#define ESP_FAIL 1
#endif

const char *esp_err_to_name(esp_err_t err);

#define NVS_DEFAULT_PART_NAME "nvs"

typedef enum {
  NVS_TYPE_U8 = 0x01,   /*!< Type uint8_t */
  NVS_TYPE_I8 = 0x11,   /*!< Type int8_t */
  NVS_TYPE_U16 = 0x02,  /*!< Type uint16_t */
  NVS_TYPE_I16 = 0x12,  /*!< Type int16_t */
  NVS_TYPE_U32 = 0x04,  /*!< Type uint32_t */
  NVS_TYPE_I32 = 0x14,  /*!< Type int32_t */
  NVS_TYPE_U64 = 0x08,  /*!< Type uint64_t */
  NVS_TYPE_I64 = 0x18,  /*!< Type int64_t */
  NVS_TYPE_STR = 0x21,  /*!< Type string */
  NVS_TYPE_BLOB = 0x42, /*!< Type blob */
  NVS_TYPE_ANY = 0xff   /*!< Must be last */
} nvs_type_t;

typedef void *nvs_handle_t;

enum nvs_open_mode_t { NVS_READONLY, NVS_READWRITE };

esp_err_t nvs_flash_init();
esp_err_t nvs_flash_deinit();
esp_err_t nvs_flash_erase();
esp_err_t nvs_open(const char *namespace_name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle);
esp_err_t nvs_commit(nvs_handle_t handle);
esp_err_t nvs_close(nvs_handle_t handle);

esp_err_t nvs_erase_key(nvs_handle_t c_handle, const char *key);

esp_err_t nvs_get_i8(nvs_handle_t c_handle, const char *key, int8_t *out_value);
esp_err_t nvs_get_u8(nvs_handle_t c_handle, const char *key, uint8_t *out_value);
esp_err_t nvs_get_i16(nvs_handle_t c_handle, const char *key, int16_t *out_value);
esp_err_t nvs_get_u16(nvs_handle_t c_handle, const char *key, uint16_t *out_value);
esp_err_t nvs_get_i32(nvs_handle_t c_handle, const char *key, int32_t *out_value);
esp_err_t nvs_get_u32(nvs_handle_t c_handle, const char *key, uint32_t *out_value);
esp_err_t nvs_get_i64(nvs_handle_t c_handle, const char *key, int64_t *out_value);
esp_err_t nvs_get_u64(nvs_handle_t c_handle, const char *key, uint64_t *out_value);
esp_err_t nvs_get_str(nvs_handle_t c_handle, const char *key, char *out_value, size_t *length);
esp_err_t nvs_get_blob(nvs_handle_t c_handle, const char *key, void *out_value, size_t *length);

esp_err_t nvs_set_i8(nvs_handle_t handle, const char *key, int8_t value);
esp_err_t nvs_set_u8(nvs_handle_t handle, const char *key, uint8_t value);
esp_err_t nvs_set_i16(nvs_handle_t handle, const char *key, int16_t value);
esp_err_t nvs_set_u16(nvs_handle_t handle, const char *key, uint16_t value);
esp_err_t nvs_set_i32(nvs_handle_t handle, const char *key, int32_t value);
esp_err_t nvs_set_u32(nvs_handle_t handle, const char *key, uint32_t value);
esp_err_t nvs_set_i64(nvs_handle_t handle, const char *key, int64_t value);
esp_err_t nvs_set_u64(nvs_handle_t handle, const char *key, uint64_t value);
esp_err_t nvs_set_str(nvs_handle_t c_handle, const char *key, const char *value);
esp_err_t nvs_set_blob(nvs_handle_t c_handle, const char *key, const void *value, size_t length);

namespace nvs {
namespace internal {

esp_err_t nvs_set(nvs_handle_t handle, const char *key, const void *value, size_t value_size);
esp_err_t nvs_get(nvs_handle_t handle, const char *key, void *value, size_t *value_size);
esp_err_t nvs_item_get_size(nvs_handle_t handle, const char *key, size_t *value_size);

}  // namespace internal
}  // namespace nvs
