#include <fstream>
#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"
#include "jpeg_wrapper.h"

class MozjpegEncodeBench : public BenchmarkImplementation {
 public:
  std::string name() const override { return "mozjpeg-encode"; }

  void prepare(const Args &args) override {
    // Load input file (PPM format expected)
    std::ifstream file(args.input, std::ios::binary | std::ios::ate);
    if (!file)
      throw std::runtime_error("Failed to open input file: " + args.input);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size))
      throw std::runtime_error("Failed to read input file");

    // Simple PPM parser (P6 format)
    const char *data = buffer.data();
    if (buffer.size() < 3 || data[0] != 'P' || data[1] != '6') {
      throw std::runtime_error("Input must be PPM P6 format");
    }

    // Skip to dimensions
    size_t pos = 3;
    while (pos < buffer.size() &&
           (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r'))
      pos++;

    // Skip comments
    while (pos < buffer.size() && data[pos] == '#') {
      while (pos < buffer.size() && data[pos] != '\n') pos++;
      pos++;
    }

    // Read width and height
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

    // Skip max value
    while (pos < buffer.size() &&
           (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r'))
      pos++;
    while (pos < buffer.size() && data[pos] >= '0' && data[pos] <= '9') pos++;
    while (pos < buffer.size() &&
           (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r'))
      pos++;

    // Copy pixel data
    input_data.assign(data + pos, data + buffer.size());

    if (input_data.size() < width * height * 3) {
      throw std::runtime_error("Insufficient pixel data in PPM file");
    }

    // Store quality setting
    if (args.quality == "web-low") {
      quality = 50;
      progressive = false;
    } else if (args.quality == "web-high") {
      quality = 80;
      progressive = true;
    } else {  // archival
      quality = 95;
      progressive = false;
    }
  }

  std::vector<uint8_t> run(const Args &args) override {
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    unsigned char *outbuffer = nullptr;
    unsigned long outsize = 0;

    jpeg_mem_dest(&cinfo, &outbuffer, &outsize);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    if (progressive) {
      jpeg_simple_progression(&cinfo);
    }

    // Set subsampling for archival (4:4:4)
    if (args.quality == "archival") {
      cinfo.comp_info[0].h_samp_factor = 1;
      cinfo.comp_info[0].v_samp_factor = 1;
      cinfo.comp_info[1].h_samp_factor = 1;
      cinfo.comp_info[1].v_samp_factor = 1;
      cinfo.comp_info[2].h_samp_factor = 1;
      cinfo.comp_info[2].v_samp_factor = 1;
    }

    jpeg_start_compress(&cinfo, TRUE);

    int row_stride = width * 3;
    while (cinfo.next_scanline < cinfo.image_height) {
      uint8_t *row_pointer = const_cast<uint8_t *>(
          input_data.data() + cinfo.next_scanline * row_stride);
      jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);

    std::vector<uint8_t> output(outbuffer, outbuffer + outsize);

    if (outbuffer) {
      free(outbuffer);
    }

    jpeg_destroy_compress(&cinfo);

    return output;
  }

  void verify(const Args &args, const std::vector<uint8_t> &output) override {
    // Basic verification: check that output is non-empty and is valid JPEG
    if (output.empty()) {
      throw std::runtime_error("Encoder produced empty output");
    }
    if (output.size() < 2 || output[0] != 0xFF || output[1] != 0xD8) {
      throw std::runtime_error(
          "Output is not a valid JPEG (missing SOI marker)");
    }
  }

 private:
  std::vector<uint8_t> input_data;
  int width;
  int height;
  int quality;
  bool progressive;
};

int main(int argc, char **argv) {
  MozjpegEncodeBench bench;
  return run_benchmark(argc, argv, bench);
}
