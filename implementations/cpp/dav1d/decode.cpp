#include <avif/avif.h>
#include <dav1d/dav1d.h>

#include <cstring>
#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"

class Dav1dBench : public BenchmarkImplementation {
 public:
  std::string name() const override { return "dav1d-decode"; }

  void prepare(const Args &args) override {
    // Use libavif to extract the raw AV1 payload from the AVIF container
    input_data = read_binary_file(args.input);
    threads = args.threads;
  }

  std::vector<uint8_t> run(const Args &args) override {
    return decode(input_data);
  }

 private:
  std::vector<uint8_t> decode(const std::vector<uint8_t> &data) {
    avifDecoder *decoder = avifDecoderCreate();
    if (!decoder) throw std::runtime_error("avifDecoderCreate failed");

    // Force dav1d AV1 decoder for a fair comparison against libavif-decode (aom)
    avifDecoderSetCodecChoice(decoder, AVIF_CODEC_CHOICE_DAV1D);
    if (threads > 0) {
      decoder->maxThreads = threads;
    }

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

    // Collect RGB pixels
    std::vector<uint8_t> rgb_data;
    rgb_data.reserve(rgb.width * rgb.height * 3);
    for (uint32_t y = 0; y < rgb.height; ++y) {
      uint8_t *row = rgb.pixels + (y * rgb.rowBytes);
      rgb_data.insert(rgb_data.end(), row, row + (rgb.width * 3));
    }

    avifRGBImageFreePixels(&rgb);

    return encode_ppm_rgb8(rgb.width, rgb.height, rgb_data);
  }

  std::vector<uint8_t> input_data;
  int threads = 0;
};

int main(int argc, char **argv) {
  Dav1dBench bench;
  return run_benchmark(argc, argv, bench);
}
