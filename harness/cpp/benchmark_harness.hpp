#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <chrono>
#include <fstream>
#include <cstring>
#include <memory>
#include <algorithm>
#include <thread>
#include <cmath>
#include <stdexcept>
#include <zlib.h>

struct Args {
    std::string input;
    std::string output;
    std::string quality;
    int iterations = 10;
    int warmup = 2;
    int threads = 0;
    bool discard = false;
    bool verify = false;
    bool preallocate = false;
    double verify_threshold = 60.0;
};

class BenchmarkImplementation {
public:
    virtual ~BenchmarkImplementation() = default;
    virtual std::string name() const = 0;
    virtual void prepare(const Args& args) = 0;
    virtual std::vector<uint8_t> run(const Args& args) = 0;
    virtual void verify(const Args& args, const std::vector<uint8_t>& output) = 0;
};

// Calculate PSNR between two byte arrays
inline double calculate_psnr(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    if (a.size() != b.size()) {
        throw std::runtime_error("Image size mismatch for PSNR calculation");
    }
    
    double mse = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        double diff = static_cast<double>(a[i]) - static_cast<double>(b[i]);
        mse += diff * diff;
    }
    mse /= a.size();
    
    if (mse == 0.0) {
        return INFINITY;
    }
    
    return 10.0 * std::log10(255.0 * 255.0 / mse);
}

// Verify lossless output (exact byte match)
inline void verify_lossless(const std::vector<uint8_t>& output, const std::vector<uint8_t>& reference) {
    if (output.size() != reference.size()) {
        throw std::runtime_error("Lossless verification failed: size mismatch");
    }
    
    if (output != reference) {
        // Find first difference
        for (size_t i = 0; i < output.size(); ++i) {
            if (output[i] != reference[i]) {
                std::ostringstream oss;
                oss << "Lossless verification failed: byte mismatch at offset " << i;
                throw std::runtime_error(oss.str());
            }
        }
    }
}

// Verify lossy output (PSNR-based)
inline void verify_lossy(const std::vector<uint8_t>& output, const std::vector<uint8_t>& reference, double threshold_db) {
    double psnr = calculate_psnr(output, reference);
    
    if (psnr < threshold_db) {
        std::ostringstream oss;
        oss << "Lossy verification failed: PSNR " << psnr << " dB below threshold " << threshold_db << " dB";
        throw std::runtime_error(oss.str());
    }
}

inline uint32_t crc32_hash(const std::vector<uint8_t>& data) {
    // Use zlib's hardware-accelerated CRC32 implementation
    return ::crc32(0L, data.data(), data.size());
}

inline Args parse_args(int argc, char** argv) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--input" && i + 1 < argc) args.input = argv[++i];
        else if (arg == "--output" && i + 1 < argc) args.output = argv[++i];
        else if (arg == "--quality" && i + 1 < argc) args.quality = argv[++i];
        else if (arg == "--iterations" && i + 1 < argc) args.iterations = std::stoi(argv[++i]);
        else if (arg == "--warmup" && i + 1 < argc) args.warmup = std::stoi(argv[++i]);
        else if (arg == "--threads" && i + 1 < argc) args.threads = std::stoi(argv[++i]);
        else if (arg == "--discard") args.discard = true;
        else if (arg == "--verify") args.verify = true;
        else if (arg == "--preallocate") args.preallocate = true;
        else if (arg == "--verify-threshold" && i + 1 < argc) args.verify_threshold = std::stod(argv[++i]);
    }
    return args;
}

inline int run_benchmark(int argc, char** argv, BenchmarkImplementation& impl) {
    Args args = parse_args(argc, argv);

    if (args.threads > 0) {
        // Set environment variable for OMP
        std::string threads_str = std::to_string(args.threads);
#ifdef _WIN32
        _putenv_s("OMP_NUM_THREADS", threads_str.c_str());
#else
        setenv("OMP_NUM_THREADS", threads_str.c_str(), 1);
#endif
    }

    try {
        impl.prepare(args);

        // Warmup
        for (int i = 0; i < args.warmup; ++i) {
            impl.run(args);
        }

        // Measurement
        for (int i = 0; i < args.iterations; ++i) {
            auto output = impl.run(args);

            if (args.discard) {
                uint32_t c = crc32_hash(output);
                // Prevent optimization
                volatile uint32_t dummy = c;
                (void)dummy;
            } else {
                if (!output.empty()) {
                    std::ofstream outfile(args.output, std::ios::binary);
                    outfile.write(reinterpret_cast<const char*>(output.data()), output.size());
                }
            }

            if (args.verify) {
                impl.verify(args, output);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
