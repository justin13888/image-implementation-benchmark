#include <jxl/decode.h>
#include <jxl/decode_cxx.h>
#include <jxl/resizable_parallel_runner.h>
#include <jxl/resizable_parallel_runner_cxx.h>

#include <stdexcept>
#include <vector>

#include "benchmark_harness.hpp"

class LibJxlBench : public BenchmarkImplementation {
 public:
  std::string name() const override { return "libjxl-decode"; }

  void prepare(const Args &args) override {
    input_data = read_binary_file(args.input);

    // Create thread pool once; reused across all decode() iterations.
    // Use args.threads when specified (> 0), otherwise let the runner decide.
    runner = JxlResizableParallelRunnerMake(nullptr);
    size_t num_threads = args.threads > 0
                             ? static_cast<size_t>(args.threads)
                             : JxlResizableParallelRunnerSuggestThreads(
                                   input_data.size(), 0);
    JxlResizableParallelRunnerSetThreads(runner.get(), num_threads);
  }

  std::vector<uint8_t> run(const Args &args) override {
    return decode(args, input_data);
  }

 private:
  std::vector<uint8_t> decode(const Args &args,
                              const std::vector<uint8_t> &data) {
    auto dec = JxlDecoderMake(nullptr);

    if (JXL_DEC_SUCCESS !=
        JxlDecoderSubscribeEvents(dec.get(),
                                  JXL_DEC_BASIC_INFO | JXL_DEC_FULL_IMAGE)) {
      throw std::runtime_error("JxlDecoderSubscribeEvents failed");
    }

    if (JXL_DEC_SUCCESS !=
        JxlDecoderSetParallelRunner(dec.get(), JxlResizableParallelRunner,
                                    runner.get())) {
      throw std::runtime_error("JxlDecoderSetParallelRunner failed");
    }

    JxlDecoderSetInput(dec.get(), data.data(), data.size());
    JxlDecoderCloseInput(dec.get());

    JxlBasicInfo info;
    JxlPixelFormat format = {3, JXL_TYPE_UINT8, JXL_LITTLE_ENDIAN, 0};

    std::vector<uint8_t> output;

    for (;;) {
      JxlDecoderStatus status = JxlDecoderProcessInput(dec.get());

      if (status == JXL_DEC_ERROR) {
        throw std::runtime_error("JxlDecoderProcessInput failed");
      } else if (status == JXL_DEC_NEED_MORE_INPUT) {
        throw std::runtime_error("Expected more input");
      } else if (status == JXL_DEC_BASIC_INFO) {
        if (JXL_DEC_SUCCESS != JxlDecoderGetBasicInfo(dec.get(), &info)) {
          throw std::runtime_error("JxlDecoderGetBasicInfo failed");
        }
        // Only update thread count from image dimensions when --threads was not
        // explicitly set; otherwise keep the user-specified thread count.
        if (args.threads <= 0) {
          JxlResizableParallelRunnerSetThreads(
              runner.get(),
              JxlResizableParallelRunnerSuggestThreads(info.xsize, info.ysize));
        }
      } else if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER) {
        size_t buffer_size;
        if (JXL_DEC_SUCCESS !=
            JxlDecoderImageOutBufferSize(dec.get(), &format, &buffer_size)) {
          throw std::runtime_error("JxlDecoderImageOutBufferSize failed");
        }
        output.resize(buffer_size);
        if (JXL_DEC_SUCCESS != JxlDecoderSetImageOutBuffer(dec.get(), &format,
                                                           output.data(),
                                                           output.size())) {
          throw std::runtime_error("JxlDecoderSetImageOutBuffer failed");
        }
      } else if (status == JXL_DEC_FULL_IMAGE) {
        // Nothing to do
      } else if (status == JXL_DEC_SUCCESS) {
        break;
      } else {
        // Unknown status
      }
    }

    return encode_ppm_rgb8(info.xsize, info.ysize, output);
  }

  std::vector<uint8_t> input_data;
  JxlResizableParallelRunnerPtr runner{nullptr};
};

int main(int argc, char **argv) {
  LibJxlBench bench;
  return run_benchmark(argc, argv, bench);
}
