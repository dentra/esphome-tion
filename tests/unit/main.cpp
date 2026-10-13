#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"

#include <cstdio>
#include <cstdlib>

#include "harness.h"

namespace dentra {
namespace tion {

// overrides weak millis() from tion-api
uint32_t millis() { return test::FakeClock::now(); }

}  // namespace tion
}  // namespace dentra

uint32_t test::FakeClock::now_ = 0;

int main(int argc, char **argv) {
  // set TION_LOG=1 to see tion-api log output
  if (std::getenv("TION_LOG") != nullptr) {
    dentra::tion::set_logger([](int level, const char *tag, int line, const char *format, va_list args) {
      std::fprintf(stderr, "[%d][%s:%d] ", level, tag, line);
      std::vfprintf(stderr, format, args);
      std::fputc('\n', stderr);
    });
  }

  doctest::Context context;
  context.applyCommandLine(argc, argv);
  return context.run();
}
