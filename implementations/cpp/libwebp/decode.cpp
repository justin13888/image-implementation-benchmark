#include "benchmark_harness.hpp"
#include <webp/decode.h>
#include <vector>
#include <stdexcept>
#include <fstream>

class LibWebpBench : public BenchmarkImplementation {
public:
    std::string name() const override { return "libwebp-decode"; }

    void prepare(const Args& args) override {
        std::ifstream file(args.input, std::ios::binary | std::ios::ate);
        if (!file) throw std::runtime_error("Failed to open input file: " + args.input);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        input_data.resize(size);
        if (!file.read(reinterpret_cast<char*>(input_data.data()), size))
            throw std::runtime_error("Failed to read input file");
    }

    std::vector<uint8_t> run(const Args& args) override {
        int width, height;
        uint8_t* output_buffer = WebPDecodeRGBA(input_data.data(), input_data.size(), &width, &height);
        
        if (output_buffer == nullptr) {
            throw std::runtime_error("WebPDecodeRGBA failed");
        }
        
        std::vector<uint8_t> output(output_buffer, output_buffer + width * height * 4);
        WebPFree(output_buffer);
        
        return output;
    }

    void verify(const Args& args, const std::vector<uint8_t>& output) override {
        // Verification logic
    }

private:
    std::vector<uint8_t> input_data;
};

int main(int argc, char** argv) {
    LibWebpBench bench;
    return run_benchmark(argc, argv, bench);
}
