#include "esp_system.h"

extern "C" {
uint32_t esp_get_free_heap_size(void) { return 0x800; }

void xQueueSemaphoreTake() {}
void xQueueGenericSend() {}
void xQueueCreateMutex() {}
}
