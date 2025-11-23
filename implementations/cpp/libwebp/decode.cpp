#include <webp/decode.h>

#include <fstream>
#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"

class LibWebpBench : public BenchmarkImplementation {
 public:
  std::string name() const override { return "libwebp-decode"; }

  void prepare(const Args &args) override {
    std::ifstream file(args.input, std::ios::binary | std::ios::ate);
    if (!file)
      throw std::runtime_error("Failed to open input file: " + args.input);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    input_data.resize(size);
    if (!file.read(reinterpret_cast<char *>(input_data.data()), size))
      throw std::runtime_error("Failed to read input file");

    if (args.verify) {
      reference_output = decode(input_data);
    }
  }

  std::vector<uint8_t> run(const Args &args) override {
    return decode(input_data);
  }

  void verify(const Args &args, const std::vector<uint8_t> &output) override {
    if (reference_output.empty()) {
      throw std::runtime_error(
          "Reference output not available for verification");
    }
    // WebP can be lossy or lossless.
    // For self-verification, it should be exact.
    verify_lossless(output, reference_output);
  }

 private:
  std::vector<uint8_t> decode(const std::vector<uint8_t> &data) {
    int width, height;
    uint8_t *output_buffer =
        WebPDecodeRGBA(data.data(), data.size(), &width, &height);

    if (output_buffer == nullptr) {
      throw std::runtime_error("WebPDecodeRGBA failed");
    }

    std::vector<uint8_t> output(output_buffer,
                                output_buffer + width * height * 4);
    WebPFree(output_buffer);

    return output;
  }

  std::vector<uint8_t> input_data;
  std::vector<uint8_t> reference_output;
};

int main(int argc, char **argv) {
  LibWebpBench bench;
  return run_benchmark(argc, argv, bench);
}
