#include "avif_encode_impl.hpp"

class LibAvifEncodeBench : public AvifEncodeBenchBase {
 public:
  LibAvifEncodeBench() : AvifEncodeBenchBase(AVIF_CODEC_CHOICE_AOM) {}
  std::string name() const override { return "libavif-encode"; }
};

int main(int argc, char **argv) {
  LibAvifEncodeBench bench;
  return run_benchmark(argc, argv, bench);
}
