#include "log.h"

namespace dentra {
namespace tion {
#ifndef TION_ESPHOME
static logger_fn_t logger_ = nullptr;
void set_logger(const logger_fn_t &&logger) { logger_ = std::move(logger); }

void __attribute__((hot)) tion_log_printf_(int level, const char *tag, int line, const char *format, ...) {
  if (logger_ == nullptr) {
    return;
  }
  va_list arg;
  va_start(arg, format);
  logger_(level, tag, line, format, arg);
  va_end(arg);
}
#endif
}  // namespace tion
}  // namespace dentra
