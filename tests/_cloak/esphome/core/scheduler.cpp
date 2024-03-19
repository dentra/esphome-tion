#include "log.h"
#include "component.h"
#include "application.h"

namespace esphome {

static const char *const TAG = "scheduler";

void Scheduler::test_timeout(Component *component, bool start) {
  auto it = this->test_timeouts_.find(component);
  if (start) {
    if (it == this->test_timeouts_.end()) {
      this->test_timeouts_.emplace(component, TimeoutFunc{});
    }
  } else {
    if (it != this->test_timeouts_.end()) {
      for (auto [k, v] : it->second) {
        v();
      }
      this->test_timeouts_.erase(it);
    }
  }
}

void Scheduler::set_timeout(Component *component, const std::string &name, uint32_t timeout,
                            std::function<void()> func) {
  auto it = this->test_timeouts_.find(component);
  if (it != this->test_timeouts_.end()) {
    it->second.emplace(name, std::move(func));
  } else {
    func();
  }
}

bool Scheduler::cancel_timeout(Component *component, const std::string &name) {
  auto it = this->test_timeouts_.find(component);
  if (it != this->test_timeouts_.end()) {
    return it->second.erase(name) != 0;
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
