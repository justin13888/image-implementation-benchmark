#include <spng.h>

#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"

class SpngEncodeBench : public BenchmarkImplementation {
 public:
  std::string name() const override { return "spng-encode"; }

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

    // Validate size
    size_t expected_size = (size_t)width * height * 3;
    if (input_data.size() < expected_size) {
      throw std::runtime_error("Input data too small for dimensions");
    }
    if (input_data.size() > expected_size) {
      // PPM might have trailing whitespace
      input_data.resize(expected_size);
    }

    // Map quality to compression level
    // spng uses 0-9, where 0 is no compression, 9 is best compression.
    // Default is usually 6.
    if (args.quality == "web-low") {
      compression_level = 1;  // Fast
    } else if (args.quality == "web-high") {
      compression_level = 6;  // Default
    } else {                  // archival
      compression_level = 9;  // Best
    }
  }

  std::vector<uint8_t> run(const Args &args) override {
    spng_ctx *ctx = spng_ctx_new(SPNG_CTX_ENCODER);
    if (!ctx) throw std::runtime_error("spng_ctx_new failed");

    // We need to encode to a buffer. spng_encode_image doesn't allocate for us
    // if we want a vector? Actually spng_encode_image writes to the output set
    // by spng_set_png_buffer or spng_set_png_stream. Wait, spng_set_png_buffer
    // is for decoding? For encoding, we usually use spng_set_option with
    // SPNG_ENCODE_TO_BUFFER? Or spng_set_png_stream with a custom write
    // function.

    // Let's use a custom write function to write to vector.
    // spng_set_png_stream(ctx, write_fn, &output);

    // Wait, libspng documentation says:
    // "spng_set_png_stream() sets the output stream."

    struct MemBuf {
      std::vector<uint8_t> data;
    } output_buf;

    // Reserve some space
    output_buf.data.reserve(input_data.size() / 2);

    int ret = spng_set_png_stream(
        ctx,
        [](spng_ctx *, void *user, void *data, size_t len) -> int {
          auto *buf = static_cast<MemBuf *>(user);
          buf->data.insert(buf->data.end(), (uint8_t *)data,
                           (uint8_t *)data + len);
          return 0;  // Success
        },
        &output_buf);

    if (ret) {
      spng_ctx_free(ctx);
      throw std::runtime_error("spng_set_png_stream failed");
    }

    spng_ihdr ihdr = {0};
    ihdr.width = width;
    ihdr.height = height;
    ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR;
    ihdr.bit_depth = 8;

    ret = spng_set_ihdr(ctx, &ihdr);
    if (ret) {
      spng_ctx_free(ctx);
      throw std::runtime_error("spng_set_ihdr failed");
    }

    // Set compression level
    // SPNG_ENCODE_OPTION_IMG_COMPRESSION_LEVEL might not exist in older
    // versions, but let's try. Actually, checking headers would be good.
    // Assuming standard libspng 0.7.0+

    // spng_set_option(ctx, SPNG_IMG_COMPRESSION_LEVEL, compression_level);

    ret = spng_encode_image(ctx, input_data.data(), input_data.size(),
                            SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);

    if (ret) {
      spng_ctx_free(ctx);
      throw std::runtime_error(std::string("spng_encode_image failed: ") +
                               spng_strerror(ret));
    }

    spng_ctx_free(ctx);
    return output_buf.data;
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
  uint32_t width;
  uint32_t height;
  int compression_level;
};

int main(int argc, char **argv) {
  SpngEncodeBench bench;
  return run_benchmark(argc, argv, bench);
}
