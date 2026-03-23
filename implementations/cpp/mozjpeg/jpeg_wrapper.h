// Prevent clang-format from reordering includes
// clang-format off
#include <cstddef>
#include <cstdio>
#include <jpeglib.h>
#include <jconfig.h>
// clang-format on

// Compile-time guard: mozjpeg does not define C_ARITH_CODING_SUPPORTED;
// libjpeg-turbo does.
#ifdef C_ARITH_CODING_SUPPORTED
#error \
    "This implementation must be compiled against mozjpeg, not libjpeg-turbo. Check CMAKE_PREFIX_PATH."
#endif
