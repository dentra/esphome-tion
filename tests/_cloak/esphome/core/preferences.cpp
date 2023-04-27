#include "preferences.h"

namespace esphome {

class TestPreferenceBackend : public ESPPreferenceBackend {
 public:
  bool save(const uint8_t *data, size_t len) override { return false; }
  bool load(uint8_t *data, size_t len) override { return false; }
};

class TestPrefs : public ESPPreferences {
 public:
  ESPPreferenceObject make_preference(size_t length, uint32_t type, bool in_flash) override {
    // TestPreferenceBackend
    return ESPPreferenceObject(&backend_);
  }
  ESPPreferenceObject make_preference(size_t length, uint32_t type) override {
    // return TestPreferenceBackend
    return ESPPreferenceObject(&backend_);
  }

  bool sync() override { return true; }
  bool reset() override { return true; }

 protected:
  TestPreferenceBackend backend_;
};
TestPrefs prefs;

ESPPreferences *global_preferences = &prefs;
}  // namespace esphome
