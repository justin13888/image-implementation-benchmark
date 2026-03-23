#include "jpeg_decode_impl.hpp"
#include "jpeg_wrapper.h"

class MozJpegBench : public JpegDecodeBenchBase {
 public:
  std::string name() const override { return "mozjpeg-decode"; }
};

int main(int argc, char **argv) {
  MozJpegBench bench;
  return run_benchmark(argc, argv, bench);
}
