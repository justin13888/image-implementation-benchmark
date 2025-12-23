#pragma once

#include <zlib.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

struct Args {
  std::string input;
  std::string output;
  std::string quality;
  int iterations = 10;
  int warmup = 2;
  int threads = 0;
  bool discard = false;
};

class BenchmarkImplementation {
 public:
  virtual ~BenchmarkImplementation() = default;
  virtual std::string name() const = 0;
  virtual void prepare(const Args& args) = 0;
  virtual std::vector<uint8_t> run(const Args& args) = 0;
};

inline uint32_t crc32_hash(const std::vector<uint8_t>& data) {
  // Use zlib's hardware-accelerated CRC32 implementation
  return ::crc32(0L, data.data(), data.size());
}

inline Args parse_args(int argc, char** argv) {
  Args args;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--input" && i + 1 < argc)
      args.input = argv[++i];
    else if (arg == "--output" && i + 1 < argc)
      args.output = argv[++i];
    else if (arg == "--quality" && i + 1 < argc)
      args.quality = argv[++i];
    else if (arg == "--iterations" && i + 1 < argc)
      args.iterations = std::stoi(argv[++i]);
    else if (arg == "--warmup" && i + 1 < argc)
      args.warmup = std::stoi(argv[++i]);
    else if (arg == "--threads" && i + 1 < argc)
      args.threads = std::stoi(argv[++i]);
    else if (arg == "--discard")
      args.discard = true;
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
          outfile.write(reinterpret_cast<const char*>(output.data()),
                        output.size());
        }
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
