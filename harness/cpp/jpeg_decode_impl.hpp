// Shared JPEG decode implementation for libjpeg-turbo and mozjpeg.
// Each decode.cpp must also include its own jpeg_wrapper.h for the
// compile-time library guard.
#pragma once

// clang-format off
#include <cstddef>
#include <cstdio>
#include <jpeglib.h>
// clang-format on
#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"

class JpegDecodeBenchBase : public BenchmarkImplementation {
 public:
  void prepare(const Args &args) override {
    input_data = read_binary_file(args.input);
  }

  std::vector<uint8_t> run(const Args &args) override {
    jpeg_decompress_struct cinfo;
    jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, input_data.data(), input_data.size());

    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
      jpeg_destroy_decompress(&cinfo);
      throw std::runtime_error("Failed to read JPEG header");
    }

    jpeg_start_decompress(&cinfo);

    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int components = cinfo.output_components;

    std::vector<uint8_t> output(width * height * components);

    while (cinfo.output_scanline < cinfo.output_height) {
      uint8_t *buffer_array[1];
      buffer_array[0] =
          output.data() + cinfo.output_scanline * width * components;
      jpeg_read_scanlines(&cinfo, buffer_array, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return encode_ppm_rgb8(width, height, output);
  }

 private:
  std::vector<uint8_t> input_data;
};
