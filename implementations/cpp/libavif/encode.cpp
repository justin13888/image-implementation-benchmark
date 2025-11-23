#include <avif/avif.h>

#include <fstream>
#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"

class LibAvifEncodeBench : public BenchmarkImplementation {
 public:
  std::string name() const override { return "libavif-encode"; }

  void prepare(const Args &args) override {
    // Load input PPM file
    std::ifstream file(args.input, std::ios::binary | std::ios::ate);
    if (!file)
      throw std::runtime_error("Failed to open input file: " + args.input);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size))
      throw std::runtime_error("Failed to read input file");

    // Parse PPM
    const char *data = buffer.data();
    if (buffer.size() < 3 || data[0] != 'P' || data[1] != '6') {
      throw std::runtime_error("Input must be PPM P6 format");
    }

    size_t pos = 3;
    while (pos < buffer.size() &&
           (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r'))
      pos++;
    while (pos < buffer.size() && data[pos] == '#') {
      while (pos < buffer.size() && data[pos] != '\n') pos++;
      pos++;
    }

    width = 0;
    while (pos < buffer.size() && data[pos] >= '0' && data[pos] <= '9') {
      width = width * 10 + (data[pos] - '0');
      pos++;
    }
    while (pos < buffer.size() &&
           (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r'))
      pos++;

    height = 0;
    while (pos < buffer.size() && data[pos] >= '0' && data[pos] <= '9') {
      height = height * 10 + (data[pos] - '0');
      pos++;
    }

    while (pos < buffer.size() &&
           (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r'))
      pos++;
    while (pos < buffer.size() && data[pos] >= '0' && data[pos] <= '9') pos++;
    while (pos < buffer.size() &&
           (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r'))
      pos++;

    input_data.assign(data + pos, data + buffer.size());

    // Configure quality settings per README spec
    if (args.quality == "web-low") {
      quality = 65;
      speed = 6;
      yuv_format = AVIF_PIXEL_FORMAT_YUV420;
    } else if (args.quality == "web-high") {
      quality = 65;
      speed = AVIF_SPEED_DEFAULT;
      yuv_format = AVIF_PIXEL_FORMAT_YUV420;
    } else {  // archival
      quality = 85;
      speed = AVIF_SPEED_DEFAULT;
      yuv_format = AVIF_PIXEL_FORMAT_YUV444;
    }
  }

  std::vector<uint8_t> run(const Args &args) override {
    avifImage *image = avifImageCreate(width, height, 8, yuv_format);
    if (!image) {
      throw std::runtime_error("avifImageCreate failed");
    }

    // Convert RGB to YUV
    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, image);
    rgb.format = AVIF_RGB_FORMAT_RGB;
    rgb.pixels = const_cast<uint8_t *>(input_data.data());
    rgb.rowBytes = width * 3;

    if (avifImageRGBToYUV(image, &rgb) != AVIF_RESULT_OK) {
      avifImageDestroy(image);
      throw std::runtime_error("avifImageRGBToYUV failed");
    }

    avifEncoder *encoder = avifEncoderCreate();
    if (!encoder) {
      avifImageDestroy(image);
      throw std::runtime_error("avifEncoderCreate failed");
    }

    encoder->quality = quality;
    encoder->qualityAlpha = quality;
    encoder->speed = speed;

    avifRWData output = AVIF_DATA_EMPTY;
    avifResult result = avifEncoderWrite(encoder, image, &output);

    std::vector<uint8_t> output_vec;
    if (result == AVIF_RESULT_OK) {
      output_vec.assign(output.data, output.data + output.size);
    }

    avifRWDataFree(&output);
    avifEncoderDestroy(encoder);
    avifImageDestroy(image);

    if (result != AVIF_RESULT_OK) {
      throw std::runtime_error("avifEncoderWrite failed");
    }

    return output_vec;
  }

  void verify(const Args &args, const std::vector<uint8_t> &output) override {
    if (output.empty()) {
      throw std::runtime_error("Encoder produced empty output");
    }
  }

 private:
  std::vector<uint8_t> input_data;
  int width;
  int height;
  int quality;
  int speed;
  avifPixelFormat yuv_format;
};

int main(int argc, char **argv) {
  LibAvifEncodeBench bench;
  return run_benchmark(argc, argv, bench);
}
