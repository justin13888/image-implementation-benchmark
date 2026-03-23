#include "avif_decode_impl.hpp"

class LibAvifBench : public AvifDecodeBenchBase {
 public:
  // Force aom AV1 decoder for a fair comparison against dav1d-decode
  LibAvifBench() : AvifDecodeBenchBase(AVIF_CODEC_CHOICE_AOM) {}
  std::string name() const override { return "libavif-decode"; }
};

int main(int argc, char **argv) {
  LibAvifBench bench;
  return run_benchmark(argc, argv, bench);
}
