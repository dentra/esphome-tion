#include "component.h"
#include "application.h"

namespace esphome {

static const char *const TAG = "component";

namespace setup_priority {

const float BUS = 1000.0f;
const float IO = 900.0f;
const float HARDWARE = 800.0f;
const float DATA = 600.0f;
const float PROCESSOR = 400.0;
const float BLUETOOTH = 350.0f;
const float AFTER_BLUETOOTH = 300.0f;
const float WIFI = 250.0f;
const float BEFORE_CONNECTION = 220.0f;
const float AFTER_WIFI = 200.0f;
const float AFTER_CONNECTION = 100.0f;
const float LATE = -100.0f;

}  // namespace setup_priority

const uint32_t COMPONENT_STATE_MASK = 0xFF;
const uint32_t COMPONENT_STATE_CONSTRUCTION = 0x00;
const uint32_t COMPONENT_STATE_SETUP = 0x01;
const uint32_t COMPONENT_STATE_LOOP = 0x02;
const uint32_t COMPONENT_STATE_FAILED = 0x03;
const uint32_t STATUS_LED_MASK = 0xFF00;
const uint32_t STATUS_LED_OK = 0x0000;
const uint32_t STATUS_LED_WARNING = 0x0100;
const uint32_t STATUS_LED_ERROR = 0x0200;

void Component::test_timeout(bool start) { App.scheduler.test_timeout(this, start); }
void Component::set_timeout(const std::string &name, uint32_t timeout, std::function<void()> &&f) {
  App.scheduler.set_timeout(this, name, timeout, std::move(f));
}

void Component::set_timeout(uint32_t timeout, std::function<void()> &&f) {
  this->set_timeout("__default__", timeout, std::move(f));
}

bool Component::cancel_timeout(const std::string &name) { return App.scheduler.cancel_timeout(this, name); }

}  // namespace esphome
