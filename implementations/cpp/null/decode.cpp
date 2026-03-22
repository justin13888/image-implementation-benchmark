#include <cstdint>
#include <cstring>
#include <vector>

#include "../../../harness/cpp/benchmark_harness.hpp"

class NullBench : public BenchmarkImplementation {
 private:
  std::vector<uint8_t> input_data;

 public:
  std::string name() const override { return "null-decode"; }

  void prepare(const Args &args) override {
    input_data = read_binary_file(args.input);
  }

  std::vector<uint8_t> run(const Args &args) override {
    std::vector<uint8_t> output(input_data);

    // Write checksum at end to prevent optimization
    uint32_t checksum = crc32_hash(input_data);
    if (output.size() >= 4) {
      std::memcpy(&output[output.size() - 4], &checksum, 4);
    }

    return output;
  }
};

int main(int argc, char **argv) {
  NullBench bench;
  return run_benchmark(argc, argv, bench);
}
