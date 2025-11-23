#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <fstream>
#include <cstring>
#include <memory>
#include <algorithm>
#include <thread>

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

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
};

class BenchmarkImplementation {
public:
    virtual ~BenchmarkImplementation() = default;
    virtual std::string name() const = 0;
    virtual void prepare(const Args& args) = 0;
    virtual std::vector<uint8_t> run(const Args& args) = 0;
    virtual void verify(const Args& args, const std::vector<uint8_t>& output) = 0;
};

inline uint32_t crc32(const std::vector<uint8_t>& data) {
    uint32_t crc = 0;
    // Simple software CRC32 or use hardware if available.
    // For now, let's use a simple one or the intrinsic if we are sure about the platform.
    // The README mentions _mm_crc32_u64.
    
    // Using _mm_crc32_u8 for byte-by-byte or u64 for blocks.
    // Note: This requires -msse4.2
    
    size_t i = 0;
    // Align to 8 bytes
    // ... (Skipping complex alignment logic for brevity, just doing simple loop for now or hardware intrinsic)
    
    // Let's use a simple loop with hardware intrinsic for x86_64
    uint64_t crc64 = 0;
    const uint8_t* p = data.data();
    size_t len = data.size();
    
    while (len >= 8) {
        crc64 = _mm_crc32_u64(crc64, *reinterpret_cast<const uint64_t*>(p));
        p += 8;
        len -= 8;
    }
    while (len > 0) {
        crc64 = _mm_crc32_u8(static_cast<uint32_t>(crc64), *p);
        p++;
        len--;
    }
    return static_cast<uint32_t>(crc64);
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
                uint32_t c = crc32(output);
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
