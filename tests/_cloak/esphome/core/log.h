#pragma once

namespace esphome {

#define LOG_STR_LITERAL(s) (s)

#define ESPHOME_LOG_LEVEL_NONE 0
#define ESPHOME_LOG_LEVEL_ERROR 1
#define ESPHOME_LOG_LEVEL_WARN 2
#define ESPHOME_LOG_LEVEL_INFO 3
#define ESPHOME_LOG_LEVEL_CONFIG 4
#define ESPHOME_LOG_LEVEL_DEBUG 5
#define ESPHOME_LOG_LEVEL_VERBOSE 6
#define ESPHOME_LOG_LEVEL_VERY_VERBOSE 7

#ifndef ESPHOME_LOG_LEVEL
#define ESPHOME_LOG_LEVEL ESPHOME_LOG_LEVEL_NONE
#endif

#define ESPHOME_LOG_COLOR_BLACK "30"
#define ESPHOME_LOG_COLOR_RED "31"     // ERROR
#define ESPHOME_LOG_COLOR_GREEN "32"   // INFO
#define ESPHOME_LOG_COLOR_YELLOW "33"  // WARNING
#define ESPHOME_LOG_COLOR_BLUE "34"
#define ESPHOME_LOG_COLOR_MAGENTA "35"  // CONFIG
#define ESPHOME_LOG_COLOR_CYAN "36"     // DEBUG
#define ESPHOME_LOG_COLOR_GRAY "37"     // VERBOSE
#define ESPHOME_LOG_COLOR_WHITE "38"
#define ESPHOME_LOG_SECRET_BEGIN "\033[5m"
#define ESPHOME_LOG_SECRET_END "\033[6m"
#define ESPHOME_LOG_COLOR(COLOR) "\033[0;" COLOR "m"
#define ESPHOME_LOG_BOLD(COLOR) "\033[1;" COLOR "m"
#define ESPHOME_LOG_RESET_COLOR "\033[0m"

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#undef ESP_LOGV

#define _ESP_LOG(LEVEL, TAG, fmt, COLOR, ...) \
  printf(ESPHOME_LOG_COLOR(COLOR) "%s " LEVEL ": " fmt ESPHOME_LOG_RESET_COLOR "\n", TAG, ##__VA_ARGS__)

#define ESP_LOGE(TAG, fmt, ...) _ESP_LOG("ERR", TAG, fmt, ESPHOME_LOG_COLOR_RED, ##__VA_ARGS__)
#define ESP_LOGW(TAG, fmt, ...) _ESP_LOG("WRN", TAG, fmt, ESPHOME_LOG_COLOR_YELLOW, ##__VA_ARGS__)
#define ESP_LOGC(TAG, fmt, ...) _ESP_LOG("CNF", TAG, fmt, ESPHOME_LOG_COLOR_MAGENTA, ##__VA_ARGS__)
#define ESP_LOGI(TAG, fmt, ...) _ESP_LOG("INF", TAG, fmt, ESPHOME_LOG_COLOR_GREEN, ##__VA_ARGS__)
#define ESP_LOGD(TAG, fmt, ...) _ESP_LOG("DBG", TAG, fmt, ESPHOME_LOG_COLOR_CYAN, ##__VA_ARGS__)
#define ESP_LOGV(TAG, fmt, ...) _ESP_LOG("VRB", TAG, fmt, ESPHOME_LOG_COLOR_GRAY, ##__VA_ARGS__)
#define ESP_LOGVV(TAG, fmt, ...) _ESP_LOG("VVB", TAG, fmt, ESPHOME_LOG_COLOR_GRAY, ##__VA_ARGS__)
#define ESP_LOGCONFIG(TAG, fmt, ...) ESP_LOGC(TAG, fmt, ##__VA_ARGS__)
#define ESPHOME_LOG_HAS_VERY_VERBOSE
#define ESPHOME_LOG_HAS_VERBOSE
#define ESPHOME_LOG_HAS_DEBUG
#define ESPHOME_LOG_HAS_CONFIG
#define ESPHOME_LOG_HAS_INFO
#define ESPHOME_LOG_HAS_WARN
#define ESPHOME_LOG_HAS_ERROR

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte) \
  ((byte) &0x80 ? '1' : '0'), ((byte) &0x40 ? '1' : '0'), ((byte) &0x20 ? '1' : '0'), ((byte) &0x10 ? '1' : '0'), \
      ((byte) &0x08 ? '1' : '0'), ((byte) &0x04 ? '1' : '0'), ((byte) &0x02 ? '1' : '0'), ((byte) &0x01 ? '1' : '0')
#define YESNO(b) ((b) ? "YES" : "NO")
#define ONOFF(b) ((b) ? "ON" : "OFF")
#define TRUEFALSE(b) ((b) ? "TRUE" : "FALSE")

#define esp_idf_log_vprintf_ vprintf
#define esp_log_set_vprintf(x)
#define esp_log_level_set(x, y)

// Helper class that identifies strings that may be stored in flash storage (similar to Arduino's __FlashStringHelper)
struct LogString;
#define LOG_STR(s) (reinterpret_cast<const LogString *>(s))
#define LOG_STR_ARG(s) (reinterpret_cast<const char *>(s))
#define LOG_STR_LITERAL(s) (s)

}  // namespace esphome
