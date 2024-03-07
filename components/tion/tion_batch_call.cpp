#include "esphome/core/log.h"
#include "esphome/core/application.h"

#include "tion_batch_call.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_batch_call";
static const char *const BATCH_TIMEOUT = "batch_timeout";

void BatchStateCall::perform() {
  if (this->timeout_) {
    App.scheduler.set_timeout(this->c_, BATCH_TIMEOUT, this->timeout_, [this]() { this->perform_(); });
  } else {
    this->perform_();
  }
}

void BatchStateCall::perform_() {
  ESP_LOGD(TAG, "Write out batch changes");
  dentra::tion::TionStateCall::perform();
  // defer call
  App.scheduler.set_timeout(this->c_, "", 0, [write_out = std::move(this->write_out_)]() { write_out(); });
}

}  // namespace tion
}  // namespace esphome
