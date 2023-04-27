#pragma once
#include <cerrno>

typedef struct {
  int tz_minuteswest; /* minutes west of Greenwich */
  int tz_dsttime;     /* type of dst correction */
} timezone_t;

inline int settimeofday(void *tv, void *tz) { return ECANCELED; }
