#pragma once

#define TION_LOG_LEVEL_NONE 0
#define TION_LOG_LEVEL_ERROR 1
#define TION_LOG_LEVEL_WARN 2
#define TION_LOG_LEVEL_INFO 3
#define TION_LOG_LEVEL_CONFIG 4
#define TION_LOG_LEVEL_DEBUG 5
#define TION_LOG_LEVEL_VERBOSE 6

#ifdef TION_ESPHOME
#include "esphome/core/log.h"
using namespace esphome;
#define TION_LOGV ESP_LOGV
#define TION_LOGD ESP_LOGD
#define TION_LOGC ESP_LOGC
#define TION_LOGI ESP_LOGI
#define TION_LOGW ESP_LOGW
#define TION_LOGE ESP_LOGE

#ifndef TION_LOG_LEVEL
#define TION_LOG_LEVEL ESPHOME_LOG_LEVEL
#endif

#else  // TION_ESPHOME

#include <functional>  // std::function
#include <stdarg.h>    // va_list

#ifndef TION_LOG_LEVEL
#define TION_LOG_LEVEL TION_LOG_LEVEL_DEBUG
#endif

namespace dentra {
namespace tion {

using logger_fn_t = std::function<void(int, const char *, int, const char *, va_list)>;
void set_logger(const logger_fn_t &&logger);

void tion_log_printf_(int level, const char *tag, int line, const char *format, ...)  // NOLINT
    __attribute__((format(printf, 4, 5)));
#endif

#ifndef TION_LOGV
#if TION_LOG_LEVEL >= TION_LOG_LEVEL_VERBOSE
#define TION_LOGV(tag, format, ...) tion_log_printf_(TION_LOG_LEVEL_VERBOSE, tag, __LINE__, format, ##__VA_ARGS__)
#else
#define TION_LOGV(tag, format, ...)
#endif
#endif

#ifndef TION_LOGD
#if TION_LOG_LEVEL >= TION_LOG_LEVEL_DEBUG
#define TION_LOGD(tag, format, ...) tion_log_printf_(TION_LOG_LEVEL_DEBUG, tag, __LINE__, format, ##__VA_ARGS__)
#else
#define TION_LOGD(tag, format, ...)
#endif
#endif

#ifndef TION_LOGI
#if TION_LOG_LEVEL >= TION_LOG_LEVEL_INFO
#define TION_LOGI(tag, format, ...) tion_log_printf_(TION_LOG_LEVEL_INFO, tag, __LINE__, format, ##__VA_ARGS__)
#else
#define TION_LOGI(tag, format, ...)
#endif
#endif

#ifndef TION_LOGW
#if TION_LOG_LEVEL >= TION_LOG_LEVEL_WARN
#define TION_LOGW(tag, format, ...) tion_log_printf_(TION_LOG_LEVEL_WARN, tag, __LINE__, format, ##__VA_ARGS__)
#else
#define TION_LOGW(tag, format, ...)
#endif
#endif

#ifndef TION_LOGE
#if TION_LOG_LEVEL >= TION_LOG_LEVEL_ERROR
#define TION_LOGE(tag, format, ...) tion_log_printf_(TION_LOG_LEVEL_ERROR, tag, __LINE__, format, ##__VA_ARGS__)
#else
#define TION_LOGE(tag, format, ...)
#endif

}  // namespace tion
}  // namespace dentra

#endif  // TION_ESPHOME
