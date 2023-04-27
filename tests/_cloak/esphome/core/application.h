#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/preferences.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/scheduler.h"

namespace esphome {

class Application {
 public:
  Scheduler scheduler;
  void reboot() {}
  void feed_wdt() {}
};

/// Global storage of Application pointer - only one Application can exist.
extern Application App;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

}  // namespace esphome
