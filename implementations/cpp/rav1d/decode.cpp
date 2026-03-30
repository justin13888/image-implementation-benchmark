#include "avif_decode_impl.hpp"

class Rav1dBench : public AvifDecodeBenchBase {
 public:
  // AVIF_CODEC_CHOICE_DAV1D routes through libavif's codec_dav1d.c,
  // but rav1d provides the dav1d_* symbols at link time
  Rav1dBench() : AvifDecodeBenchBase(AVIF_CODEC_CHOICE_DAV1D) {}
  std::string name() const override { return "rav1d-decode"; }
};

int main(int argc, char **argv) {
  Rav1dBench bench;
  return run_benchmark(argc, argv, bench);
}
