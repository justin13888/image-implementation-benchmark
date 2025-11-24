#include <cstdint>
#include <fstream>
#include <vector>

#include "../../../harness/cpp/benchmark_harness.hpp"

class NullEncodeBench : public BenchmarkImplementation {
 private:
  std::vector<uint8_t> input_data;
  std::vector<uint8_t> output_buffer;

  uint32_t crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
      crc ^= data[i];
      for (int j = 0; j < 8; j++) {
        crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
      }
    }
    return ~crc;
  }

 public:
  std::string name() const override { return "null-encode"; }

  void prepare(const Args &args) override {
    std::ifstream file(args.input, std::ios::binary | std::ios::ate);
    if (!file) {
      throw std::runtime_error("Failed to open input file");
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    input_data.resize(size);
    file.read(reinterpret_cast<char *>(input_data.data()), size);

    // Preallocate output buffer to same size
    output_buffer.resize(size);
  }

  std::vector<uint8_t> run(const Args &args) override {
    // Just compute CRC32 and copy to output buffer
    uint32_t checksum = crc32(input_data.data(), input_data.size());
    std::copy(input_data.begin(), input_data.end(), output_buffer.begin());

    // Write checksum at end to prevent optimization
    if (output_buffer.size() >= 4) {
      std::memcpy(&output_buffer[output_buffer.size() - 4], &checksum, 4);
    }

    return output_buffer;
  }

  void verify(const Args &args, const std::vector<uint8_t> &output) override {
    // No verification needed for null benchmark
  }
};

int main(int argc, char **argv) {
  NullEncodeBench bench;
  return run_benchmark(argc, argv, bench);
}
