#include <cstdlib>
#include <ctime>
#include <cstring>
#include "utils.h"

DEFINE_TAG;

uint8_t fast_random_8() {
  // std::random_device r;
  // std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
  // std::mt19937 eng{seed};
  // std::uniform_int_distribution<> dist(0, 255);
  // return dist(eng);

  static bool init = false;
  if (!init) {
    std::srand(std::time(0));
    init = !init;
  }

  constexpr auto minValue = 0;
  constexpr auto maxValue = 255;
  return std::rand() % (maxValue - minValue + 1) + minValue;
}
