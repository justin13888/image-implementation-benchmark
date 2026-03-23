#include "jpeg_decode_impl.hpp"
#include "jpeg_wrapper.h"

class LibJpegBench : public JpegDecodeBenchBase {
 public:
  std::string name() const override { return "libjpeg-turbo-decode"; }
};

int main(int argc, char **argv) {
  LibJpegBench bench;
  return run_benchmark(argc, argv, bench);
}
