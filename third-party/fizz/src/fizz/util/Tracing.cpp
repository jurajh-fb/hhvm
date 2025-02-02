#include <fizz/util/Tracing.h>
#include <folly/tracing/StaticTracepoint.h>

namespace fizz {
extern "C" {
void fizz_probe_secret_available(
    long unsigned int secretSize,
    unsigned char* secretData,
    fizz::KeyLogWriter::Label nssLabel,
    unsigned char* clientRandom) {
  FOLLY_SDT(
      fizz,
      fizz_secret_available,
      secretSize,
      secretData,
      nssLabel,
      clientRandom);
}
}

} // namespace fizz
