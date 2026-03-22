#include <webp/decode.h>

#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"

class LibWebpBench : public BenchmarkImplementation {
 public:
  std::string name() const override { return "libwebp-decode"; }

  void prepare(const Args &args) override {
    input_data = read_binary_file(args.input);
  }

  std::vector<uint8_t> run(const Args &args) override {
    return decode(input_data);
  }

 private:
  std::vector<uint8_t> decode(const std::vector<uint8_t> &data) {
    int width, height;
    uint8_t *output_buffer =
        WebPDecodeRGB(data.data(), data.size(), &width, &height);

    if (output_buffer == nullptr) {
      throw std::runtime_error("WebPDecodeRGB failed");
    }

    std::vector<uint8_t> output(output_buffer,
                                output_buffer + width * height * 3);
    WebPFree(output_buffer);

    return encode_ppm_rgb8(width, height, output);
  }

  std::vector<uint8_t> input_data;
};

int main(int argc, char **argv) {
  LibWebpBench bench;
  return run_benchmark(argc, argv, bench);
}
