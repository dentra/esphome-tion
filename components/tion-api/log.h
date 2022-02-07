#pragma once

#include <functional>  // std::function
#include <stdarg.h>    // va_list

#define TION_LOG_LEVEL_NONE 0
#define TION_LOG_LEVEL_ERROR 1
#define TION_LOG_LEVEL_WARN 2
#define TION_LOG_LEVEL_INFO 3
#define TION_LOG_LEVEL_CONFIG 4
#define TION_LOG_LEVEL_DEBUG 5
#define TION_LOG_LEVEL_VERBOSE 6

#ifdef TION_ESPHOME
#define TION_LOG_LEVEL ESPHOME_LOG_LEVEL
#include "esphome/core/log.h"
#define TION_LOGV ESP_LOGV
#define TION_LOGD ESP_LOGD
#define TION_LOGC ESP_LOGC
#define TION_LOGI ESP_LOGI
#define TION_LOGW ESP_LOGW
#define TION_LOGE ESP_LOGE
using namespace esphome;
#endif

#ifndef TION_LOG_LEVEL
#define TION_LOG_LEVEL TION_LOG_LEVEL_DEBUG
#endif

namespace dentra {
namespace tion {

#ifdef TION_ESPHOME
#define tion_log_printf_ esp_log_printf_
#else
using logger_fn_t = std::function<void(int, const char *, int, const char *, va_list)>;
void set_logger(const logger_fn_t &&logger);

void tion_log_printf_(int level, const char *tag, int line, const char *format, ...)  // NOLINT
    __attribute__((format(printf, 4, 5)));
#endif

#ifndef TION_LOGV
#if TION_LOG_LEVEL >= TION_LOG_LEVEL_VERBOSE
#define tion_log_v(tag, format, ...) tion_log_printf_(TION_LOG_LEVEL_VERBOSE, tag, __LINE__, format, ##__VA_ARGS__)
#else
#define tion_log_v(tag, format, ...)
#endif
#define TION_LOGV(tag, ...) tion_log_v(tag, __VA_ARGS__)
#endif

#ifndef TION_LOGD
#if TION_LOG_LEVEL >= TION_LOG_LEVEL_DEBUG
#define tion_log_d(tag, format, ...) tion_log_printf_(TION_LOG_LEVEL_DEBUG, tag, __LINE__, format, ##__VA_ARGS__)
#else
#define tion_log_d(tag, format, ...)
#endif
#define TION_LOGD(tag, ...) tion_log_d(tag, __VA_ARGS__)
#endif

#ifndef TION_LOGI
#if TION_LOG_LEVEL >= TION_LOG_LEVEL_INFO
#define tion_log_i(tag, format, ...) tion_log_printf_(TION_LOG_LEVEL_INFO, tag, __LINE__, format, ##__VA_ARGS__)
#else
#define tion_log_i(tag, format, ...)
#endif
#define TION_LOGI(tag, ...) tion_log_i(tag, __VA_ARGS__)
#endif

#ifndef TION_LOGW
#if TION_LOG_LEVEL >= TION_LOG_LEVEL_WARN
#define tion_log_w(tag, format, ...) tion_log_printf_(TION_LOG_LEVEL_WARN, tag, __LINE__, format, ##__VA_ARGS__)
#else
#define tion_log_w(tag, format, ...)
#endif
#define TION_LOGW(tag, ...) tion_log_w(tag, __VA_ARGS__)
#endif

#ifndef TION_LOGE
#if TION_LOG_LEVEL >= TION_LOG_LEVEL_ERROR
#define tion_log_e(tag, format, ...) tion_log_printf_(TION_LOG_LEVEL_ERROR, tag, __LINE__, format, ##__VA_ARGS__)
#else
#define tion_log_e(tag, format, ...)
#endif
#define TION_LOGE(tag, ...) tion_log_e(tag, __VA_ARGS__)
#endif

}  // namespace tion
}  // namespace dentra
