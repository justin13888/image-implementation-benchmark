#include <png.h>

#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"

class LibPngEncodeBench : public BenchmarkImplementation {
 public:
  std::string name() const override { return "libpng-encode"; }

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

    // PNG is lossless, compression level varies by quality tier
    if (args.quality == "web-low") {
      compression_level = Z_BEST_SPEED;
    } else if (args.quality == "web-high") {
      compression_level = Z_DEFAULT_COMPRESSION;
    } else {  // archival
      compression_level = Z_BEST_COMPRESSION;
    }
  }

  std::vector<uint8_t> run(const Args &args) override {
    std::vector<uint8_t> output;

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                  nullptr, nullptr, nullptr);
    if (!png_ptr) throw std::runtime_error("png_create_write_struct failed");

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
      png_destroy_write_struct(&png_ptr, nullptr);
      throw std::runtime_error("png_create_info_struct failed");
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
      png_destroy_write_struct(&png_ptr, &info_ptr);
      throw std::runtime_error("PNG encoding error");
    }

    // Custom write function
    png_set_write_fn(
        png_ptr, &output,
        [](png_structp png_ptr, png_bytep data, png_size_t length) {
          auto *vec =
              static_cast<std::vector<uint8_t> *>(png_get_io_ptr(png_ptr));
          vec->insert(vec->end(), data, data + length);
        },
        [](png_structp) {});

    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_set_compression_level(png_ptr, compression_level);

    png_write_info(png_ptr, info_ptr);

    for (int y = 0; y < height; y++) {
      png_write_row(png_ptr,
                    const_cast<uint8_t *>(input_data.data() + y * width * 3));
    }

    png_write_end(png_ptr, nullptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    return output;
  }

  void verify(const Args &args, const std::vector<uint8_t> &output) override {
    if (output.empty()) {
      throw std::runtime_error("Encoder produced empty output");
    }
    // Check PNG signature
    if (output.size() < 8 || output[0] != 0x89 || output[1] != 'P' ||
        output[2] != 'N' || output[3] != 'G') {
      throw std::runtime_error("Output is not a valid PNG");
    }
  }

 private:
  std::vector<uint8_t> input_data;
  int width;
  int height;
  int compression_level;
};

int main(int argc, char **argv) {
  LibPngEncodeBench bench;
  return run_benchmark(argc, argv, bench);
}
