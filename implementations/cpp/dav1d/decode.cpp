#include "benchmark_harness.hpp"
#include <dav1d/dav1d.h>
#include <avif/avif.h>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <cstring>

class Dav1dBench : public BenchmarkImplementation {
public:
    std::string name() const override { return "dav1d-decode"; }

    void prepare(const Args& args) override {
        // Use libavif to extract the raw AV1 payload
        std::ifstream file(args.input, std::ios::binary | std::ios::ate);
        if (!file) throw std::runtime_error("Failed to open input file: " + args.input);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> avif_data(size);
        if (!file.read(reinterpret_cast<char*>(avif_data.data()), size))
            throw std::runtime_error("Failed to read input file");

        // We just store the AVIF data. The run loop uses libavif to decode it.
        input_data = std::move(avif_data);

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
        avifDecoder * decoder = avifDecoderCreate();
        if (!decoder) throw std::runtime_error("avifDecoderCreate failed");

        // Ensure we clean up
        struct DecoderGuard {
            avifDecoder* d;
            ~DecoderGuard() { avifDecoderDestroy(d); }
        } guard{decoder};

        avifResult result = avifDecoderSetIOMemory(decoder, data.data(), data.size());
        if (result != AVIF_RESULT_OK) {
            throw std::runtime_error("avifDecoderSetIOMemory failed");
        }

        result = avifDecoderParse(decoder);
        if (result != AVIF_RESULT_OK) {
            throw std::runtime_error("avifDecoderParse failed");
        }

        result = avifDecoderNextImage(decoder);
        if (result != AVIF_RESULT_OK) {
            throw std::runtime_error("avifDecoderNextImage failed");
        }
        
        // We have YUV in decoder->image.
        // We need to return something.
        // Let's just copy the Y plane.
        
        size_t size = decoder->image->yuvRowBytes[0] * decoder->image->height;
        std::vector<uint8_t> output(size);
        memcpy(output.data(), decoder->image->yuvPlanes[0], size);
        
        return output;
    }

    std::vector<uint8_t> input_data;
    std::vector<uint8_t> reference_output;
};

int main(int argc, char** argv) {
    Dav1dBench bench;
    return run_benchmark(argc, argv, bench);
}
