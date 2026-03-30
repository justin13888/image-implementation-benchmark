#include "avif_encode_impl.hpp"

class SvtAv1EncodeBench : public AvifEncodeBenchBase {
 public:
  SvtAv1EncodeBench() : AvifEncodeBenchBase(AVIF_CODEC_CHOICE_SVT) {}
  std::string name() const override { return "svt-av1-encode"; }
};

int main(int argc, char **argv) {
  SvtAv1EncodeBench bench;
  return run_benchmark(argc, argv, bench);
}
