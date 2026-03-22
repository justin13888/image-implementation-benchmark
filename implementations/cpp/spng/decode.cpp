#include <spng.h>

#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"

class SpngBench : public BenchmarkImplementation {
 public:
  std::string name() const override { return "spng-decode"; }

  void prepare(const Args &args) override {
    input_data = read_binary_file(args.input);
  }

  std::vector<uint8_t> run(const Args &args) override {
    return decode(input_data);
  }

 private:
  std::vector<uint8_t> decode(const std::vector<uint8_t> &data) {
    spng_ctx *ctx = spng_ctx_new(0);
    if (!ctx) throw std::runtime_error("spng_ctx_new failed");

    // Ensure cleanup
    struct CtxGuard {
      spng_ctx *c;
      ~CtxGuard() { spng_ctx_free(c); }
    } guard{ctx};

    if (spng_set_png_buffer(ctx, data.data(), data.size())) {
      throw std::runtime_error("spng_set_png_buffer failed");
    }

    struct spng_ihdr ihdr;
    if (spng_get_ihdr(ctx, &ihdr)) {
      throw std::runtime_error("spng_get_ihdr failed");
    }

    size_t out_size;
    if (spng_decoded_image_size(ctx, SPNG_FMT_RGB8, &out_size)) {
      throw std::runtime_error("spng_decoded_image_size failed");
    }

    std::vector<uint8_t> rgb_data(out_size);
    if (spng_decode_image(ctx, rgb_data.data(), out_size, SPNG_FMT_RGB8, 0)) {
      throw std::runtime_error("spng_decode_image failed");
    }

    return encode_ppm_rgb8(ihdr.width, ihdr.height, rgb_data);
  }

  std::vector<uint8_t> input_data;
};

int main(int argc, char **argv) {
  SpngBench bench;
  return run_benchmark(argc, argv, bench);
}
