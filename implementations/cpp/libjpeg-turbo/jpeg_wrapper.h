// Prevent clang-format from reordering includes
// clang-format off
#include <cstddef>
#include <cstdio>
#include <jpeglib.h>
#include <jconfig.h>
// clang-format on

// Compile-time guard: libjpeg-turbo defines C_ARITH_CODING_SUPPORTED; mozjpeg does not.
#ifndef C_ARITH_CODING_SUPPORTED
#  error "This implementation must be compiled against libjpeg-turbo, not mozjpeg. Check CMAKE_PREFIX_PATH."
#endif
