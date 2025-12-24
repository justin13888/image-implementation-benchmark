#include <avif/avif.h>
#include <dav1d/dav1d.h>

#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"

class Dav1dBench : public BenchmarkImplementation {
 public:
  std::string name() const override { return "dav1d-decode"; }

  void prepare(const Args &args) override {
    // Use libavif to extract the raw AV1 payload
    std::ifstream file(args.input, std::ios::binary | std::ios::ate);
    if (!file)
      throw std::runtime_error("Failed to open input file: " + args.input);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> avif_data(size);
    if (!file.read(reinterpret_cast<char *>(avif_data.data()), size))
      throw std::runtime_error("Failed to read input file");

    input_data = std::move(avif_data);
  }

  std::vector<uint8_t> run(const Args &args) override {
    return decode(input_data);
  }

 private:
  std::vector<uint8_t> decode(const std::vector<uint8_t> &data) {
    avifDecoder *decoder = avifDecoderCreate();
    if (!decoder) throw std::runtime_error("avifDecoderCreate failed");

    // Ensure we clean up
    struct DecoderGuard {
      avifDecoder *d;
      ~DecoderGuard() { avifDecoderDestroy(d); }
    } guard{decoder};

    avifResult result =
        avifDecoderSetIOMemory(decoder, data.data(), data.size());
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

    // Convert to RGB
    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, decoder->image);
    rgb.format = AVIF_RGB_FORMAT_RGB;
    rgb.depth = 8;

    avifRGBImageAllocatePixels(&rgb);
    avifResult conversionResult = avifImageYUVToRGB(decoder->image, &rgb);
    if (conversionResult != AVIF_RESULT_OK) {
      avifRGBImageFreePixels(&rgb);
      throw std::runtime_error("avifImageYUVToRGB failed");
    }

    std::string header = "P6\n" + std::to_string(rgb.width) + " " +
                         std::to_string(rgb.height) + "\n255\n";
    std::vector<uint8_t> output;
    output.reserve(header.size() + rgb.width * rgb.height * 3);
    output.insert(output.end(), header.begin(), header.end());

    for (uint32_t y = 0; y < rgb.height; ++y) {
      uint8_t *row = rgb.pixels + (y * rgb.rowBytes);
      output.insert(output.end(), row, row + (rgb.width * 3));
    }

    avifRGBImageFreePixels(&rgb);

    return output;
  }

  std::vector<uint8_t> input_data;
};

int main(int argc, char **argv) {
  Dav1dBench bench;
  return run_benchmark(argc, argv, bench);
}
