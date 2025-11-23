#include "benchmark_harness.hpp"
#include "fpng.h"
#include <png.h>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <cstring>

class FpngBench : public BenchmarkImplementation {
public:
    std::string name() const override { return "fpng"; }

    // Helper to read PNG using libpng
    void read_png(const std::string& filename, std::vector<uint8_t>& out_data, uint32_t& w, uint32_t& h, uint32_t& c) {
        FILE *fp = fopen(filename.c_str(), "rb");
        if (!fp) throw std::runtime_error("Failed to open " + filename);

        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png) throw std::runtime_error("png_create_read_struct failed");

        png_infop info = png_create_info_struct(png);
        if (!info) throw std::runtime_error("png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png))) throw std::runtime_error("libpng error");

        png_init_io(png, fp);
        png_read_info(png, info);

        w = png_get_image_width(png, info);
        h = png_get_image_height(png, info);
        png_byte color_type = png_get_color_type(png, info);
        png_byte bit_depth = png_get_bit_depth(png, info);

        if (bit_depth == 16) png_set_strip_16(png);
        if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
        if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
        if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);

        png_read_update_info(png, info);
        
        // We want RGB or RGBA
        c = png_get_channels(png, info); // Should be 3 or 4 now

        out_data.resize(w * h * c);
        std::vector<png_bytep> rows(h);
        for(uint32_t y = 0; y < h; y++) rows[y] = out_data.data() + y * w * c;

        png_read_image(png, rows.data());
        png_read_end(png, NULL);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
    }

    void prepare(const Args& args) override {
        fpng::fpng_init();
        
        // 1. Decode input PNG using libpng to get raw pixels
        std::vector<uint8_t> raw_pixels;
        uint32_t w, h, c;
        read_png(args.input, raw_pixels, w, h, c);

        // 2. Encode using fpng to memory
        // fpng expects 3 or 4 channels
        if (c != 3 && c != 4) throw std::runtime_error("Unsupported channel count for fpng: " + std::to_string(c));
        
        if (!fpng::fpng_encode_image_to_memory(raw_pixels.data(), w, h, c, input_data)) {
             throw std::runtime_error("fpng_encode_image_to_memory failed");
        }
    }

    std::vector<uint8_t> run(const Args& args) override {
        std::vector<uint8_t> out_image;
        uint32_t width, height, channels;
        
        int ret = fpng::fpng_decode_memory(input_data.data(), input_data.size(), out_image, width, height, channels, 4);
        
        if (ret != fpng::FPNG_DECODE_SUCCESS) {
            throw std::runtime_error("fpng_decode_memory failed: " + std::to_string(ret));
        }
        
        return out_image;
    }

    void verify(const Args& args, const std::vector<uint8_t>& output) override {
        // Verification logic
    }

private:
    std::vector<uint8_t> input_data;
};

int main(int argc, char** argv) {
    FpngBench bench;
    return run_benchmark(argc, argv, bench);
}
