#include "benchmark_harness.hpp"
#include <spng.h>
#include <vector>
#include <stdexcept>
#include <fstream>

class SpngBench : public BenchmarkImplementation {
public:
    std::string name() const override { return "spng-decode"; }

    void prepare(const Args& args) override {
        std::ifstream file(args.input, std::ios::binary | std::ios::ate);
        if (!file) throw std::runtime_error("Failed to open input file: " + args.input);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        input_data.resize(size);
        if (!file.read(reinterpret_cast<char*>(input_data.data()), size))
            throw std::runtime_error("Failed to read input file");

        if (args.verify) {
            reference_output = decode(input_data);
        }
    }

    std::vector<uint8_t> run(const Args& args) override {
        return decode(input_data);
    }

    void verify(const Args& args, const std::vector<uint8_t>& output) override {
        if (reference_output.empty()) {
            throw std::runtime_error("Reference output not available for verification");
        }
        verify_lossless(output, reference_output);
    }

private:
    std::vector<uint8_t> decode(const std::vector<uint8_t>& data) {
        spng_ctx *ctx = spng_ctx_new(0);
        if (!ctx) throw std::runtime_error("spng_ctx_new failed");

        // Ensure cleanup
        struct CtxGuard {
            spng_ctx* c;
            ~CtxGuard() { spng_ctx_free(c); }
        } guard{ctx};

        if (spng_set_png_buffer(ctx, data.data(), data.size())) {
            throw std::runtime_error("spng_set_png_buffer failed");
        }

        size_t out_size;
        if (spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &out_size)) {
            throw std::runtime_error("spng_decoded_image_size failed");
        }

        std::vector<uint8_t> output(out_size);
        if (spng_decode_image(ctx, output.data(), out_size, SPNG_FMT_RGBA8, 0)) {
            throw std::runtime_error("spng_decode_image failed");
        }

        return output;
    }

    std::vector<uint8_t> input_data;
    std::vector<uint8_t> reference_output;
};

int main(int argc, char** argv) {
    SpngBench bench;
    return run_benchmark(argc, argv, bench);
}
