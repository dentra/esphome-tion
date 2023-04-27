#pragma once

// #define portTICK_PERIOD_MS 1
// using TickType_t = unsigned int;
using SemaphoreHandle_t = unsigned int;

inline SemaphoreHandle_t xSemaphoreCreateMutex() { return {}; }
inline bool xSemaphoreTake(SemaphoreHandle_t &, int) { return true; }
inline void xSemaphoreGive(SemaphoreHandle_t &) {}
