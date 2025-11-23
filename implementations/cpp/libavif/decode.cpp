#include "benchmark_harness.hpp"
#include <avif/avif.h>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <cstring>

class LibAvifBench : public BenchmarkImplementation {
public:
    std::string name() const override { return "libavif-decode"; }

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
        // AVIF is lossy, but self-verification should be exact.
        verify_lossless(output, reference_output);
    }

private:
    std::vector<uint8_t> decode(const std::vector<uint8_t>& data) {
        avifDecoder * decoder = avifDecoderCreate();
        if (!decoder) throw std::runtime_error("avifDecoderCreate failed");
        
        avifResult result = avifDecoderSetIOMemory(decoder, data.data(), data.size());
        if (result != AVIF_RESULT_OK) {
            avifDecoderDestroy(decoder);
            throw std::runtime_error("avifDecoderSetIOMemory failed");
        }

        result = avifDecoderParse(decoder);
        if (result != AVIF_RESULT_OK) {
            avifDecoderDestroy(decoder);
            throw std::runtime_error("avifDecoderParse failed");
        }

        result = avifDecoderNextImage(decoder);
        if (result != AVIF_RESULT_OK) {
            avifDecoderDestroy(decoder);
            throw std::runtime_error("avifDecoderNextImage failed");
        }

        // Convert to RGB
        avifRGBImage rgb;
        avifRGBImageSetDefaults(&rgb, decoder->image);
        rgb.format = AVIF_RGB_FORMAT_RGBA;
        rgb.depth = 8;

        avifRGBImageAllocatePixels(&rgb);
        result = avifImageYUVToRGB(decoder->image, &rgb);
        if (result != AVIF_RESULT_OK) {
            avifRGBImageFreePixels(&rgb);
            avifDecoderDestroy(decoder);
            throw std::runtime_error("avifImageYUVToRGB failed");
        }

        std::vector<uint8_t> output(rgb.pixels, rgb.pixels + rgb.rowBytes * rgb.height);
        
        avifRGBImageFreePixels(&rgb);
        avifDecoderDestroy(decoder);
        
        return output;
    }

    std::vector<uint8_t> input_data;
    std::vector<uint8_t> reference_output;
};

int main(int argc, char** argv) {
    LibAvifBench bench;
    return run_benchmark(argc, argv, bench);
}
