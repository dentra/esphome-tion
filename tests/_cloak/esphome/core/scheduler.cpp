#include "log.h"
#include "component.h"
#include "application.h"

namespace esphome {

static const char *const TAG = "scheduler";

void Scheduler::set_timeout(Component *component, const std::string &name, uint32_t timeout,
                            std::function<void()> func) {
  if (this->test_timeout_) {
    this->test_timeouts_.emplace(name, std::move(func));
  } else {
    func();
  }
}

bool Scheduler::cancel_timeout(Component *component, const std::string &name) {
  if (this->test_timeout_) {
    return this->test_timeouts_.erase(name) != 0;
  }
  return true;
}

void Scheduler::set_interval(Component *component, const std::string &name, uint32_t interval,
                             std::function<void()> func) {
  ESP_LOGE(TAG, "Scheduler::set_interval is not implemented");
}

bool Scheduler::cancel_interval(Component *component, const std::string &name) {
  ESP_LOGE(TAG, "Scheduler::cancel_interval is not implemented");
  return false;
}

}  // namespace esphome
