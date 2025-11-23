#include "benchmark_harness.hpp"
#include <webp/encode.h>
#include <vector>
#include <stdexcept>
#include <fstream>

class LibWebpEncodeBench : public BenchmarkImplementation {
public:
    std::string name() const override { return "libwebp-encode"; }

    void prepare(const Args& args) override {
        // Load input PPM file
        std::ifstream file(args.input, std::ios::binary | std::ios::ate);
        if (!file) throw std::runtime_error("Failed to open input file: " + args.input);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size))
            throw std::runtime_error("Failed to read input file");
        
        // Parse PPM
        const char* data = buffer.data();
        if (buffer.size() < 3 || data[0] != 'P' || data[1] != '6') {
            throw std::runtime_error("Input must be PPM P6 format");
        }
        
        size_t pos = 3;
        while (pos < buffer.size() && (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r')) pos++;
        while (pos < buffer.size() && data[pos] == '#') {
            while (pos < buffer.size() && data[pos] != '\n') pos++;
            pos++;
        }
        
        width = 0;
        while (pos < buffer.size() && data[pos] >= '0' && data[pos] <= '9') {
            width = width * 10 + (data[pos] - '0');
            pos++;
        }
        while (pos < buffer.size() && (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r')) pos++;
        
        height = 0;
        while (pos < buffer.size() && data[pos] >= '0' && data[pos] <= '9') {
            height = height * 10 + (data[pos] - '0');
            pos++;
        }
        
        while (pos < buffer.size() && (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r')) pos++;
        while (pos < buffer.size() && data[pos] >= '0' && data[pos] <= '9') pos++;
        while (pos < buffer.size() && (data[pos] == ' ' || data[pos] == '\n' || data[pos] == '\r')) pos++;
        
        input_data.assign(data + pos, data + buffer.size());
        
        // Configure quality settings per README spec
        if (args.quality == "web-low") {
            quality = 50.0f;
            method = 4;
            lossless = false;
        } else if (args.quality == "web-high") {
            quality = 75.0f;
            method = 4;
            lossless = false;
        } else { // archival
            lossless = true;
            method = 6;
            quality = 100.0f;
        }
    }

    std::vector<uint8_t> run(const Args& args) override {
        WebPConfig config;
        if (!WebPConfigInit(&config)) {
            throw std::runtime_error("WebPConfigInit failed");
        }
        
        if (lossless) {
            config.lossless = 1;
            config.method = method;
            config.quality = quality;
        } else {
            config.lossless = 0;
            config.quality = quality;
            config.method = method;
        }
        
        if (!WebPValidateConfig(&config)) {
            throw std::runtime_error("Invalid WebP config");
        }
        
        WebPPicture picture;
        if (!WebPPictureInit(&picture)) {
            throw std::runtime_error("WebPPictureInit failed");
        }
        
        picture.width = width;
        picture.height = height;
        picture.use_argb = lossless;
        
        if (!WebPPictureImportRGB(&picture, input_data.data(), width * 3)) {
            WebPPictureFree(&picture);
            throw std::runtime_error("WebPPictureImportRGB failed");
        }
        
        WebPMemoryWriter writer;
        WebPMemoryWriterInit(&writer);
        picture.writer = WebPMemoryWrite;
        picture.custom_ptr = &writer;
        
        if (!WebPEncode(&config, &picture)) {
            WebPPictureFree(&picture);
            WebPMemoryWriterClear(&writer);
            throw std::runtime_error("WebPEncode failed");
        }
        
        std::vector<uint8_t> output(writer.mem, writer.mem + writer.size);
        
        WebPPictureFree(&picture);
        WebPMemoryWriterClear(&writer);
        
        return output;
    }

    void verify(const Args& args, const std::vector<uint8_t>& output) override {
        if (output.empty()) {
            throw std::runtime_error("Encoder produced empty output");
        }
        // Check WEBP signature
        if (output.size() < 12 || output[0] != 'R' || output[1] != 'I' || 
            output[2] != 'F' || output[3] != 'F') {
            throw std::runtime_error("Output is not a valid WEBP");
        }
    }

private:
    std::vector<uint8_t> input_data;
    int width;
    int height;
    float quality;
    int method;
    bool lossless;
};

int main(int argc, char** argv) {
    LibWebpEncodeBench bench;
    return run_benchmark(argc, argv, bench);
}
