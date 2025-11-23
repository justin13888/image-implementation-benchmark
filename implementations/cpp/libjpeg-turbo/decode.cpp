#include <fstream>
#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"
#include "jpeg_wrapper.h"

class LibJpegBench : public BenchmarkImplementation {
 public:
  std::string name() const override { return "libjpeg-turbo-decode"; }

  void prepare(const Args &args) override {
    // Load input file
    std::ifstream file(args.input, std::ios::binary | std::ios::ate);
    if (!file)
      throw std::runtime_error("Failed to open input file: " + args.input);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    input_data.resize(size);
    if (!file.read(reinterpret_cast<char *>(input_data.data()), size))
      throw std::runtime_error("Failed to read input file");

    // If verification is enabled, decode once to establish a baseline
    if (args.verify) {
      reference_output = decode(input_data);
    }
  }

  std::vector<uint8_t> run(const Args &args) override {
    return decode(input_data);
  }

  void verify(const Args &args, const std::vector<uint8_t> &output) override {
    // For self-verification (checking determinism), we expect exact matches.
    // However, we use verify_lossy to be consistent with the harness for lossy
    // formats. A high threshold (e.g. 99dB or effectively infinite) would imply
    // exact match. But since it's the same decoder, it SHOULD be exact.
    if (reference_output.empty()) {
      throw std::runtime_error(
          "Reference output not available for verification");
    }

    // Use verify_lossless for self-verification as it should be deterministic
    verify_lossless(output, reference_output);
  }

 private:
  std::vector<uint8_t> decode(const std::vector<uint8_t> &data) {
    jpeg_decompress_struct cinfo;
    jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, data.data(), data.size());

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

    return output;
  }

  std::vector<uint8_t> input_data;
  std::vector<uint8_t> reference_output;
};

int main(int argc, char **argv) {
  LibJpegBench bench;
  return run_benchmark(argc, argv, bench);
}
