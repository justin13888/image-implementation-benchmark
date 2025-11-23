#include <jxl/encode.h>
#include <jxl/encode_cxx.h>
#include <jxl/thread_parallel_runner.h>
#include <jxl/thread_parallel_runner_cxx.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstring>

// Simple PPM reader (P6)
std::vector<uint8_t> read_ppm(const char* filename, int& w, int& h) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        std::cerr << "Failed to open " << filename << std::endl;
        exit(1);
    }
    char buf[1024];
    if (!fgets(buf, sizeof(buf), f)) exit(1);
    if (strncmp(buf, "P6", 2) != 0) {
        std::cerr << "Not a P6 PPM" << std::endl;
        exit(1);
    }
    // Skip comments
    do {
        if (!fgets(buf, sizeof(buf), f)) exit(1);
    } while (buf[0] == '#');
    
    sscanf(buf, "%d %d", &w, &h);
    
    // Maxval
    if (!fgets(buf, sizeof(buf), f)) exit(1);
    
    std::vector<uint8_t> data(w * h * 3);
    if (fread(data.data(), 1, data.size(), f) != data.size()) {
        std::cerr << "Failed to read data" << std::endl;
        exit(1);
    }
    fclose(f);
    return data;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input.ppm> <output.jxl>" << std::endl;
        return 1;
    }

    int w, h;
    std::vector<uint8_t> pixels = read_ppm(argv[1], w, h);

    auto runner = JxlThreadParallelRunnerMake(nullptr, JxlThreadParallelRunnerDefaultNumWorkerThreads());
    auto enc = JxlEncoderMake(nullptr);

    if (JXL_ENC_SUCCESS != JxlEncoderSetParallelRunner(enc.get(), JxlThreadParallelRunner, runner.get())) {
        std::cerr << "JxlEncoderSetParallelRunner failed" << std::endl;
        return 1;
    }

    JxlPixelFormat pixel_format = {3, JXL_TYPE_UINT8, JXL_LITTLE_ENDIAN, 0};
    JxlBasicInfo basic_info;
    JxlEncoderInitBasicInfo(&basic_info);
    basic_info.xsize = w;
    basic_info.ysize = h;
    basic_info.uses_original_profile = JXL_TRUE;
    
    if (JXL_ENC_SUCCESS != JxlEncoderSetBasicInfo(enc.get(), &basic_info)) {
        std::cerr << "JxlEncoderSetBasicInfo failed" << std::endl;
        return 1;
    }

    JxlColorEncoding color_encoding = {};
    JxlColorEncodingSetToSRGB(&color_encoding, /*is_gray=*/JXL_FALSE);
    if (JXL_ENC_SUCCESS != JxlEncoderSetColorEncoding(enc.get(), &color_encoding)) {
        std::cerr << "JxlEncoderSetColorEncoding failed" << std::endl;
        return 1;
    }

    JxlEncoderFrameSettings* frame_settings = JxlEncoderFrameSettingsCreate(enc.get(), nullptr);
    JxlEncoderSetFrameLossless(frame_settings, JXL_FALSE);
    JxlEncoderFrameSettingsSetOption(frame_settings, JXL_ENC_FRAME_SETTING_EFFORT, 3); // Faster encoding

    if (JXL_ENC_SUCCESS != JxlEncoderAddImageFrame(frame_settings, &pixel_format, pixels.data(), pixels.size())) {
        std::cerr << "JxlEncoderAddImageFrame failed" << std::endl;
        return 1;
    }

    JxlEncoderCloseInput(enc.get());

    std::vector<uint8_t> compressed;
    compressed.resize(64);
    std::vector<uint8_t> next_out;
    next_out.resize(4096);
    uint8_t* next_out_char = next_out.data();
    size_t avail_out = next_out.size();

    JxlEncoderStatus process_result = JXL_ENC_NEED_MORE_OUTPUT;
    while (process_result == JXL_ENC_NEED_MORE_OUTPUT) {
        process_result = JxlEncoderProcessOutput(enc.get(), &next_out_char, &avail_out);
        if (process_result == JXL_ENC_ERROR) {
            std::cerr << "JxlEncoderProcessOutput failed" << std::endl;
            return 1;
        }
        size_t offset = next_out.size() - avail_out;
        compressed.insert(compressed.end(), next_out.data(), next_out.data() + offset);
        next_out_char = next_out.data();
        avail_out = next_out.size();
    }
    
    // Remove initial 64 bytes padding
    compressed.erase(compressed.begin(), compressed.begin() + 64);

    std::ofstream outfile(argv[2], std::ios::binary);
    outfile.write(reinterpret_cast<char*>(compressed.data()), compressed.size());

    return 0;
}
