#include <spng.h>

#include <cstring>
#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"

class SpngEncodeBench : public BenchmarkImplementation {
 public:
  std::string name() const override { return "spng-encode"; }

  void prepare(const Args &args) override {
    RGBImage img = decode_ppm_rgb8(args.input);

    // Trim any trailing bytes beyond the expected pixel data
    size_t expected_size = (size_t)img.width * img.height * 3;
    if (img.data.size() > expected_size) {
      img.data.resize(expected_size);
    }

    width = static_cast<uint32_t>(img.width);
    height = static_cast<uint32_t>(img.height);
    input_data = std::move(img.data);

    // TODO: SPNG encoder API does not expose compression level control.
    // Quality tiers cannot be differentiated for PNG encoding with SPNG.
    // Consider using libpng for quality-controlled PNG encoding.
  }

  std::vector<uint8_t> run(const Args &args) override {
    spng_ctx *ctx = spng_ctx_new(SPNG_CTX_ENCODER);
    if (!ctx) throw std::runtime_error("spng_ctx_new failed");

    struct MemBuf {
      std::vector<uint8_t> data;
    } output_buf;

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

 private:
  std::vector<uint8_t> input_data;
  uint32_t width;
  uint32_t height;
};

int main(int argc, char **argv) {
  SpngEncodeBench bench;
  return run_benchmark(argc, argv, bench);
}
