#include <jxl/encode.h>
#include <jxl/encode_cxx.h>
#include <jxl/thread_parallel_runner.h>
#include <jxl/thread_parallel_runner_cxx.h>

#include <fstream>
#include <iostream>  // Added for debug logging if needed
#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"

class LibJxlEncodeBench : public BenchmarkImplementation {
 public:
  std::string name() const override { return "libjxl-encode"; }

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
    auto skip_whitespace = [&]() {
      while (pos < buffer.size() && (data[pos] == ' ' || data[pos] == '\n' ||
                                     data[pos] == '\r' || data[pos] == '\t')) {
        pos++;
      }
    };

    auto skip_comments = [&]() {
      while (pos < buffer.size() && data[pos] == '#') {
        while (pos < buffer.size() && data[pos] != '\n') pos++;
        pos++;
      }
    };

    skip_whitespace();
    skip_comments();

    // Read Width
    width = 0;
    while (pos < buffer.size() && data[pos] >= '0' && data[pos] <= '9') {
      width = width * 10 + (data[pos] - '0');
      pos++;
    }
    skip_whitespace();
    skip_comments();  // Comments can appear between width and height

    // Read Height
    height = 0;
    while (pos < buffer.size() && data[pos] >= '0' && data[pos] <= '9') {
      height = height * 10 + (data[pos] - '0');
      pos++;
    }
    skip_whitespace();
    skip_comments();

    // Read MaxVal
    max_val = 0;
    while (pos < buffer.size() && data[pos] >= '0' && data[pos] <= '9') {
      max_val = max_val * 10 + (data[pos] - '0');
      pos++;
    }

    // P6 spec: single whitespace char follows maxval
    if (pos < buffer.size() && isspace(data[pos])) {
      pos++;
    }

    // Calculate expected raw size to avoid buffer mismatch errors
    int bytes_per_channel = (max_val > 255) ? 2 : 1;
    size_t expected_bytes = (size_t)width * height * 3 * bytes_per_channel;

    if (pos + expected_bytes > buffer.size()) {
      throw std::runtime_error("Incomplete P6 file");
    }

    // Assign EXACTLY the needed bytes.
    input_data.assign(data + pos, data + pos + expected_bytes);

    // Configure quality settings per README spec
    if (args.quality == "web-low") {
      distance = 4.0f;
      effort = 3;
      lossless = false;
    } else if (args.quality == "web-high") {
      distance = 1.0f;
      effort = 5;
      lossless = false;
    } else {  // archival
      distance = 0.0f;
      effort = 7;
      lossless = true;
    }
  }

  std::vector<uint8_t> run(const Args &args) override {
    auto runner = JxlThreadParallelRunnerMake(
        nullptr, args.threads > 0
                     ? args.threads
                     : JxlThreadParallelRunnerDefaultNumWorkerThreads());
    auto enc = JxlEncoderMake(nullptr);

    if (JXL_ENC_SUCCESS != JxlEncoderSetParallelRunner(enc.get(),
                                                       JxlThreadParallelRunner,
                                                       runner.get())) {
      throw std::runtime_error("JxlEncoderSetParallelRunner failed");
    }

    JxlPixelFormat pixel_format = {3, JXL_TYPE_UINT8, JXL_LITTLE_ENDIAN, 0};

    if (max_val > 255) {
      pixel_format.data_type = JXL_TYPE_UINT16;
      // P6 standard defines multi-byte samples as Big Endian (Most Significant
      // Byte first)
      pixel_format.endianness = JXL_BIG_ENDIAN;
    } else {
      pixel_format.data_type = JXL_TYPE_UINT8;
      pixel_format.endianness =
          JXL_LITTLE_ENDIAN;  // Endianness doesn't matter for 1 byte
    }

    JxlBasicInfo basic_info;
    JxlEncoderInitBasicInfo(&basic_info);
    basic_info.xsize = width;
    basic_info.ysize = height;
    // IMPORTANT: For 16-bit input, we usually want to keep the bit depth in
    // basic info
    if (max_val > 255) {
      basic_info.bits_per_sample = 16;
    } else {
      basic_info.bits_per_sample = 8;
    }
    basic_info.uses_original_profile = JXL_TRUE;

    if (JXL_ENC_SUCCESS != JxlEncoderSetBasicInfo(enc.get(), &basic_info)) {
      throw std::runtime_error("JxlEncoderSetBasicInfo failed");
    }

    JxlColorEncoding color_encoding = {};
    JxlColorEncodingSetToSRGB(&color_encoding, /*is_gray=*/JXL_FALSE);
    if (JXL_ENC_SUCCESS !=
        JxlEncoderSetColorEncoding(enc.get(), &color_encoding)) {
      throw std::runtime_error("JxlEncoderSetColorEncoding failed");
    }

    JxlEncoderFrameSettings *frame_settings =
        JxlEncoderFrameSettingsCreate(enc.get(), nullptr);

    if (lossless) {
      JxlEncoderSetFrameLossless(frame_settings, JXL_TRUE);
    } else {
      JxlEncoderSetFrameLossless(frame_settings, JXL_FALSE);
      JxlEncoderSetFrameDistance(frame_settings, distance);
    }

    JxlEncoderFrameSettingsSetOption(frame_settings,
                                     JXL_ENC_FRAME_SETTING_EFFORT, effort);

    if (JXL_ENC_SUCCESS !=
        JxlEncoderAddImageFrame(frame_settings, &pixel_format,
                                const_cast<uint8_t *>(input_data.data()),
                                input_data.size())) {
      throw std::runtime_error("JxlEncoderAddImageFrame failed");
    }

    JxlEncoderCloseInput(enc.get());

    std::vector<uint8_t> compressed;
    std::vector<uint8_t> chunk(4096);
    uint8_t *next_out = chunk.data();
    size_t avail_out = chunk.size();

    JxlEncoderStatus process_result = JXL_ENC_NEED_MORE_OUTPUT;
    while (process_result == JXL_ENC_NEED_MORE_OUTPUT) {
      process_result =
          JxlEncoderProcessOutput(enc.get(), &next_out, &avail_out);
      if (process_result == JXL_ENC_ERROR) {
        throw std::runtime_error("JxlEncoderProcessOutput failed");
      }
      size_t bytes_written = chunk.size() - avail_out;
      compressed.insert(compressed.end(), chunk.data(),
                        chunk.data() + bytes_written);
      next_out = chunk.data();
      avail_out = chunk.size();
    }

    return compressed;
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
  int max_val;  // Added to store bit-depth info
  float distance;
  int effort;
  bool lossless;
};

int main(int argc, char **argv) {
  LibJxlEncodeBench bench;
  return run_benchmark(argc, argv, bench);
}
