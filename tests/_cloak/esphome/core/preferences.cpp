#include <cassert>

#include "log.h"
#include "helpers.h"

#include "preferences.h"

namespace esphome {
static const char *const TAG = "preferences";
class TestPreferenceBackend : public ESPPreferenceBackend {
 public:
  TestPreferenceBackend(uint32_t type, size_t size) : type_(type), size_(size) {}
  bool save(const uint8_t *data, size_t len) override {
    ESP_LOGV(TAG, "Saving preferences %08X: %s", this->type_, format_hex_pretty(data, len).c_str());
    this->check_size_(len);
    return true;
  }
  bool load(uint8_t *data, size_t len) override {
    ESP_LOGV(TAG, "Loading preferences %08X: ", this->type_);
    this->check_size_(len);
    return false;
  }

 protected:
  uint32_t type_{};
  size_t size_{};
  void check_size_(size_t len) {
    if (len != this->size_) {
      ESP_LOGE(TAG, "pref data size %zu does not match backed size %zu", len, this->size_);
    }
    assert(len == this->size_);
  }
};

class TestPrefs : public ESPPreferences {
 public:
  ESPPreferenceObject make_preference(size_t length, uint32_t type, bool in_flash) override {
    return ESPPreferenceObject(new TestPreferenceBackend(type, length));
  }
  ESPPreferenceObject make_preference(size_t length, uint32_t type) override {
    return ESPPreferenceObject(new TestPreferenceBackend(type, length));
  }

  bool sync() override { return true; }
  bool reset() override { return true; }

 protected:
};
TestPrefs prefs;

ESPPreferences *global_preferences = &prefs;
}  // namespace esphome
