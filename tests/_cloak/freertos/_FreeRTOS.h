#pragma once
// #include "../esp32/esp_system.h"

#define _Static_assert static_assert
#define pvPortMalloc malloc
#define vPortFree free
#define xPortGetFreeHeapSize esp_get_free_heap_size
#define xPortGetMinimumEverFreeHeapSize esp_get_minimum_free_heap_size

// xPortGetFreeHeapSize
