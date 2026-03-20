#include <avif/avif.h>

#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"

class LibAvifEncodeBench : public BenchmarkImplementation {
 public:
  std::string name() const override { return "libavif-encode"; }

  void prepare(const Args &args) override {
    RGBImage img = decode_ppm_rgb8(args.input);
    width = img.width;
    height = img.height;
    input_data = std::move(img.data);

    // Configure quality settings per README spec
    if (args.quality == "web-low") {
      quality = 65;
      speed = 6;
      yuv_format = AVIF_PIXEL_FORMAT_YUV420;
    } else if (args.quality == "web-high") {
      quality = 65;
      speed = AVIF_SPEED_DEFAULT;
      yuv_format = AVIF_PIXEL_FORMAT_YUV420;
      // TODO: libavif encoder does not enable grain synthesis for web-high
      // despite the README mentioning it as a differentiating parameter.
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
