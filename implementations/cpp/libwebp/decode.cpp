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

    std::string header = "P6\n" + std::to_string(width) + " " +
                         std::to_string(height) + "\n255\n";
    std::vector<uint8_t> final_output;
    final_output.reserve(header.size() + output.size());
    final_output.insert(final_output.end(), header.begin(), header.end());
    final_output.insert(final_output.end(), output.begin(), output.end());

    return final_output;
  }

  std::vector<uint8_t> input_data;
};

int main(int argc, char **argv) {
  LibWebpBench bench;
  return run_benchmark(argc, argv, bench);
}
