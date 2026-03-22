#include <cstdint>
#include <cstring>
#include <vector>

#include "../../../harness/cpp/benchmark_harness.hpp"

class NullEncodeBench : public BenchmarkImplementation {
 private:
  std::vector<uint8_t> input_data;

 public:
  std::string name() const override { return "null-encode"; }

  void prepare(const Args &args) override {
    RGBImage img = decode_ppm_rgb8(args.input);
    input_data = std::move(img.data);
  }

  std::vector<uint8_t> run(const Args &args) override {
    std::vector<uint8_t> output(input_data);

    // Just compute CRC32 and write checksum at end to prevent optimization
    uint32_t checksum = crc32_hash(input_data);
    if (output.size() >= 4) {
      std::memcpy(&output[output.size() - 4], &checksum, 4);
    }

    return output;
  }
};

int main(int argc, char **argv) {
  NullEncodeBench bench;
  return run_benchmark(argc, argv, bench);
}
