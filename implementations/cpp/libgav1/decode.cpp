#include "avif_decode_impl.hpp"

class Libgav1Bench : public AvifDecodeBenchBase {
 public:
  Libgav1Bench() : AvifDecodeBenchBase(AVIF_CODEC_CHOICE_LIBGAV1) {}
  std::string name() const override { return "libgav1-decode"; }
};

int main(int argc, char **argv) {
  Libgav1Bench bench;
  return run_benchmark(argc, argv, bench);
}
