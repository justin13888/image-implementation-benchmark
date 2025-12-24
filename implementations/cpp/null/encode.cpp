#include <cstdint>
#include <cstring>
#include <vector>

#include "../../../harness/cpp/benchmark_harness.hpp"

class NullEncodeBench : public BenchmarkImplementation {
 private:
  std::vector<uint8_t> input_data;
  std::vector<uint8_t> output_buffer;

 public:
  std::string name() const override { return "null-encode"; }

  void prepare(const Args &args) override {
    RGBImage img = decode_ppm_rgb8(args.input);
    input_data = std::move(img.data);

    // Preallocate output buffer to same size
    output_buffer.resize(input_data.size());
  }

  std::vector<uint8_t> run(const Args &args) override {
    // Just compute CRC32 and copy to output buffer
    uint32_t checksum = crc32_hash(input_data);
    std::copy(input_data.begin(), input_data.end(), output_buffer.begin());

    // Write checksum at end to prevent optimization
    if (output_buffer.size() >= 4) {
      std::memcpy(&output_buffer[output_buffer.size() - 4], &checksum, 4);
    }

    return output_buffer;
  }
};

int main(int argc, char **argv) {
  NullEncodeBench bench;
  return run_benchmark(argc, argv, bench);
}
