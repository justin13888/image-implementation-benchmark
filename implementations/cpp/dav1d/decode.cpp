#include <dav1d/dav1d.h>

#include "avif_decode_impl.hpp"

class Dav1dBench : public AvifDecodeBenchBase {
 public:
  // Force dav1d AV1 decoder for a fair comparison against libavif-decode (aom)
  Dav1dBench() : AvifDecodeBenchBase(AVIF_CODEC_CHOICE_DAV1D) {}
  std::string name() const override { return "dav1d-decode"; }
};

int main(int argc, char **argv) {
  Dav1dBench bench;
  return run_benchmark(argc, argv, bench);
}
