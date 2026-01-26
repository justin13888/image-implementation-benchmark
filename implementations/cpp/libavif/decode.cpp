#include <avif/avif.h>

#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"

class LibAvifBench : public BenchmarkImplementation {
 public:
  std::string name() const override { return "libavif-decode"; }

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
    avifDecoder *decoder = avifDecoderCreate();
    if (!decoder) throw std::runtime_error("avifDecoderCreate failed");

    avifResult result =
        avifDecoderSetIOMemory(decoder, data.data(), data.size());
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
    rgb.format = AVIF_RGB_FORMAT_RGB;
    rgb.depth = 8;

    avifRGBImageAllocatePixels(&rgb);
    result = avifImageYUVToRGB(decoder->image, &rgb);
    if (result != AVIF_RESULT_OK) {
      avifRGBImageFreePixels(&rgb);
      avifDecoderDestroy(decoder);
      throw std::runtime_error("avifImageYUVToRGB failed");
    }

    // Collect RGB pixels
    std::vector<uint8_t> rgb_data;
    rgb_data.reserve(rgb.width * rgb.height * 3);
    for (uint32_t y = 0; y < rgb.height; ++y) {
      uint8_t *row = rgb.pixels + (y * rgb.rowBytes);
      rgb_data.insert(rgb_data.end(), row, row + (rgb.width * 3));
    }

    avifRGBImageFreePixels(&rgb);
    avifDecoderDestroy(decoder);

    return encode_ppm_rgb8(rgb.width, rgb.height, rgb_data);
  }

  std::vector<uint8_t> input_data;
};

int main(int argc, char **argv) {
  LibAvifBench bench;
  return run_benchmark(argc, argv, bench);
}
