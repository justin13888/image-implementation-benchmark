# image-implementation-benchmark

This repository contains benchmarks for various image format implementations, comparing performance across C, C++, and Rust libraries.

## Methodology

### Input Generation

The benchmarks use a tiered collection of images to test different performance characteristics. You select which dataset to use via the `--dataset` flag when running benchmarks.

#### Available Datasets

1. **KODAK (`--dataset kodak`)** — [KODAK PhotoCD dataset](http://r0k.us/graphics/kodak/) (24 images, ~0.4MP each)
   * L2/L3 cache resident images
   * Tests raw instruction throughput and vectorization efficiency
   * Natural photography with varied content

2. **DIV2K (`--dataset div2k`)** — [DIV2K dataset](https://data.vision.ee.ethz.ch/cvl/DIV2K/) (20 selected images, 2K/4K resolution)
   * Selected via `scripts/select_div2k.py` using perceptual hash diversity sampling
   * Tests memory bandwidth, allocator pressure, and large buffer performance
   * High-resolution, diverse content

3. **Pathological (`--dataset pathological`)** — Synthetic stress tests (4 images)
   * `solid_4k.png` — Solid color (tests RLE/skip optimizations)
   * `noise_4k.png` — Gaussian noise (worst-case for all compressors)
   * `screenshot_4k.png` — UI screenshot with text and flat regions
   * `alpha_gradient_4k.png` — Transparency gradient (for formats supporting alpha)

4. **Test (`--dataset test`)** — Single test file (legacy, minimal coverage)
   * For quick smoke tests only
   * Not recommended for comprehensive benchmarking

**Preparation Phase:**

* **For Encoding:** Images are taken as-is and converted to raw PPM (RGB24) or PAM (RGBA32) format.
* **For Decoding:** Images are pre-encoded using the **reference implementation** of the corresponding format at specific quality tiers.

#### Dataset Selection Strategy

Choose your dataset based on your benchmarking goals:

* **Performance Optimization (`kodak`)** — Best for micro-optimizations and instruction-level tuning. Images fit in cache, minimizing memory system variance.
* **Real-World Throughput (`div2k`)** — Best for measuring production performance. Tests memory bandwidth, allocator efficiency, and scaling behavior.
* **Edge Case Validation (`pathological`)** — Best for finding corner cases, testing worst-case performance, and validating optimizations don't break on synthetic inputs.
* **Quick Validation (`test`)** — Single-image smoke tests only. Not suitable for performance comparison.

**Recommendation:** Run `kodak` for initial development and optimization work, then validate with `div2k` and `pathological` before publishing results.

### Quality Tiers

We benchmark against three distinct use cases. The CLI accepts tier names directly (`--quality web-low`), and each binary maps these to format-specific arguments internally.

| Tier         | Intent            | JPEG                | AVIF             | JXL           | WEBP         |
| :----------- | :---------------- | :------------------ | :--------------- | :------------ | :----------- |
| **web-low**  | Thumbnail/Preview | Q50, Baseline       | Q65, Speed 6     | d4.0, e7      | Q50, m4      |
| **web-high** | Standard Delivery | Q80, Progressive    | Q65, Grain Synth | d1.0, e7      | Q75, m4      |
| **archival** | High Fidelity     | Q95, No Subsampling | Q85, YUV444      | d0 (Lossless) | Lossless, z6 |

### Benchmarking Architecture

To ensure statistically significant results and eliminate "Cold Start" bias (OS process spawning, dynamic linker loading), we use a hybrid approach:

1. **The Harness (Hyperfine):** Manages the statistical runs, warmup, and outlier detection.
2. **The Binary (Internal Loop):** Performs the actual decode/encode operation N times within a single process.

#### Binary Interface

Every encoder/decoder implementation is compiled into a standalone binary implementing this CLI:

```bash
./<binary> \
  --input <path> \
  --output <path> \
  --quality <web-low|web-high|archival> \
  --iterations <int> \
  --warmup <int> \
  --threads <int> \
  [--discard] \
  [--verify]
```

| Flag           | Description                                                                                                                       |
| :------------- | :-------------------------------------------------------------------------------------------------------------------------------- |
| `--iterations` | Number of timed operations in the measurement loop.                                                                               |
| `--warmup`     | Number of untimed iterations to run before measurement (default: 2). Warms branch predictors, allocators, and caches.             |
| `--threads`    | Number of threads to use. Use `1` for single-threaded benchmarks, `0` for "use all available cores".                              |
| `--discard`    | Discard output instead of writing to disk. Computes a CRC32 checksum to prevent dead code elimination. Isolates compute from I/O. |
| `--verify`     | (Decode only) After the timed loop, verify output correctness. See "Verification Strategy" below.                                 |

#### Memory Allocation Strategy

Memory is allocated and freed inside each iteration to simulate realistic per-request behavior. However, this introduces allocator variance as a confounding variable.

**Allocator configuration by language:**

| Language | Allocator | Notes                                                                                      |
| :------- | :-------- | :----------------------------------------------------------------------------------------- |
| C/C++    | mimalloc  | Linked explicitly via `-lmimalloc`                                                         |
| Rust     | mimalloc  | Via `mimalloc = { version = "0.1", features = ["local_dynamic_tls"] }` as global allocator |

For benchmarks where pure codec performance (excluding allocation) is desired, use the `--preallocate` flag, which reuses a single buffer across iterations.

#### Verification Strategy

The `--verify` flag checks decoded output for correctness. Since different implementations may produce slightly different results (due to IDCT rounding, SIMD optimizations, or floating-point ordering), we use format-appropriate strategies:

| Format Type                                     | Strategy                       | Threshold |
| :---------------------------------------------- | :----------------------------- | :-------- |
| **Lossless** (PNG, lossless WEBP, lossless JXL) | Exact byte match               | N/A       |
| **Lossy** (JPEG, AVIF, lossy WEBP, lossy JXL)   | PSNR against reference decoder | ≥ 60dB    |

**How it works:**

1. Before the timed loop, the reference implementation decodes the image once and stores the result.
2. After the timed loop completes, the test implementation's output is compared against this reference.
3. For lossy formats, PSNR is computed. Values below 60dB indicate meaningful divergence (not just LSB rounding differences).
4. Verification failures are logged with the measured PSNR value and the binary exits non-zero.

**Note:** Verification adds overhead and runs outside the timed section. Use `--verify` during CI or validation runs, not during performance measurement.

The threshold can be adjusted via `--verify-threshold <dB>` for formats or implementations with known acceptable divergence.

#### Discard Checksum

When `--discard` is set, output bytes are fed through a CRC32 checksum (using hardware acceleration where available: `_mm_crc32_u64` on x86, `__crc32d` on ARM). This prevents compiler elimination of the encode/decode work while adding minimal overhead (~0.5 cycles/byte).

#### Baseline Measurement

The benchmark suite includes a `null` operation binary that performs only:

1. Read input file into memory
2. Compute CRC32 checksum
3. (If not `--discard`) Write buffer to output

This establishes the I/O and measurement floor, allowing you to isolate codec overhead from system overhead.

### Threading Model

All benchmarks are run in two configurations:

1. **Single-threaded (`--threads 1`):** Measures per-core efficiency and is useful for comparing instruction-level optimization.
2. **Parallel (`--threads 0`):** Uses all available cores. Measures real-world throughput for batch processing.

Binaries are pinned to specific cores using `taskset` (Linux) or equivalent to reduce scheduling variance.

### Statistical Reporting

Results are collected via Hyperfine and reported with:

* **Median** (primary metric, robust to outliers)
* **Mean**
* **Standard deviation**
* **Min/Max**
* **95% confidence interval**

Hyperfine is configured with `--warmup 3` (process-level warmup, separate from the binary's internal `--warmup`) and `--min-runs 10`.

### Compilation Guidelines

Binaries are compiled for release with aggressive optimization.

#### Rust

We use the following profile in `Cargo.toml`:

```toml
[profile.release]
opt-level = 3
lto = "fat"
codegen-units = 1
panic = "abort"
strip = true
```

Built with `RUSTFLAGS="-C target-cpu=native"`.

#### C/C++

We use Clang for consistency:

```bash
clang/clang++ -O3 -flto=full -fwhole-program-vtables -fno-exceptions -fno-rtti \
  -fstrict-aliasing -fomit-frame-pointer -march=native -DNDEBUG
```

**Note:** `-march=native` makes binaries specific to the host machine. Results are not portable across architectures.

### Reproducibility Manifest

Every benchmark run generates a `manifest.json` containing:

```json
{
  "timestamp": "2025-01-15T10:30:00Z",
  "cpu": "Apple M3 Max",
  "cores": 14,
  "os": "macOS 15.2",
  "kernel": "Darwin 24.2.0",
  "compiler": {
    "clang": "17.0.6",
    "rustc": "1.84.0"
  },
  "libraries": {
    "libjpeg-turbo": "3.1.0",
    "mozjpeg": "4.1.5 (commit abc1234)",
    "libavif": "1.2.0",
    "dav1d": "1.5.0",
    "libjxl": "0.11.1",
    "...": "..."
  },
  "allocator": "mimalloc 2.1.2",
  "hyperfine": "1.18.0"
}
```

This manifest is committed alongside results for full reproducibility.

## Getting Started

### Prerequisites

* [uv](https://docs.astral.sh/uv/) - Python package manager (install: `curl -LsSf https://astral.sh/uv/install.sh | sh`)
* Rust toolchain ([rustup](https://rustup.rs/))
* CMake, Clang, and development libraries
* ImageMagick, hyperfine, wget, unzip

On Ubuntu/Debian:

```bash
sudo apt install build-essential clang clang-format cmake ccache libmimalloc-dev \
  libpng-dev libspng-dev libwebp-dev libavif-dev libdav1d-dev libjxl-dev \
  pkg-config nasm imagemagick hyperfine wget unzip webp libavif-bin libjxl-tools
```

### Setup

1. **Install Python dependencies**:

   ```bash
   uv sync  # Creates .venv with pillow, imagehash, numpy
   ```

2. **Download benchmark datasets** (~3.5GB):

   ```bash
   ./setup_data.sh  # Downloads KODAK, DIV2K, generates pathological tests
   ```

3. **Build implementations** (see individual implementation directories)

### Running Benchmarks

Use `./bench` with a dataset. Always specify `--dataset` (default `test` has minimal coverage):

```bash
# Recommended: KODAK dataset (24 images, cache-resident)
./bench --dataset kodak

# High-resolution testing (20 diverse 2K/4K images)
./bench --dataset div2k

# Pathological/stress testing (4 synthetic images)
./bench --dataset pathological

# Run specific formats
./bench --dataset kodak --formats jpeg,avif

# Decode-only benchmarks
./bench --dataset kodak --type decode

# Parallel benchmarks (all CPU cores)
./bench --dataset kodak --threads 0

# Discard output I/O (pure compute)
./bench --dataset kodak --discard-output

# With verification (slower)
./bench --dataset kodak --verify

# Measure memory usage
./bench --dataset div2k --measure-memory
```

### Results

Results are in `./results/<timestamp>/`:

* `summary.md` - Human-readable tables
* `raw.json` - Full Hyperfine output
* `manifest.json` - Reproducibility manifest
* `memory.csv` - Peak RSS (if `--measure-memory` used)

## Image Format Implementations

We include modern formats and their most competitive implementations.

> **Note:** HEIF is excluded due to licensing constraints and lack of competitive open implementations.

### JPEG

| Implementation    | Language | Notes                                                                                                          |
| :---------------- | :------- | :------------------------------------------------------------------------------------------------------------- |
| **libjpeg-turbo** | C        | Industry standard, SIMD-optimized                                                                              |
| **mozjpeg**       | C        | *Optimized for compression ratio, not speed.* Included for completeness; expect slower encode times by design. |
| **jpeg-decoder**  | Rust     | Pure Rust                                                                                                      |
| **zune-jpeg**     | Rust     | Highly optimized pure Rust                                                                                     |

### PNG

| Implementation | Language | Notes                                     |
| :------------- | :------- | :---------------------------------------- |
| **libpng**     | C        | Reference implementation                  |
| **spng**       | C        | "Simple PNG", speed-optimized             |
| **png**        | Rust     | Standard `image-rs` crate                 |
| **zune-png**   | Rust     | Highly optimized pure Rust implementation |

### WEBP

| Implementation | Language | Notes                         |
| :------------- | :------- | :---------------------------- |
| **libwebp**    | C        | Reference implementation      |
| **image-webp** | Rust     | Wrapper/native implementation |

### AVIF

| Implementation | Language | Notes                                     |
| :------------- | :------- | :---------------------------------------- |
| **libavif**    | C        | Reference (AOM/dav1d backend)             |
| **dav1d**      | C/Asm    | Direct decoder (bypasses libavif wrapper) |
| **rav1e**      | Rust     | Encoder                                   |

<!-- | **rav1d**      | Rust     | Port of dav1d. Approaching stability as of late 2025. | -->

### JPEG XL

| Implementation  | Language | Notes                         |
| :-------------- | :------- | :---------------------------- |
| **libjxl**      | C++      | Reference implementation      |
| **jxl-oxide**   | Rust     | Pure Rust decoder             |
| **zune-jpegxl** | Rust     | Optimized Rust implementation |

## Limitations and Caveats

1. **Architecture-specific results.** Due to `-march=native`, results are only valid for the exact CPU used. Cross-machine comparisons require recompilation and re-running.

2. **Allocator as confounding variable.** While we standardize on mimalloc, real-world performance may differ with system allocators.

3. **Image set limitations.** KODAK is compositionally narrow (natural photography). While we supplement with pathological cases, results may not generalize to all image types (e.g., medical imaging, satellite imagery).

4. **mozjpeg design goals.** mozjpeg prioritizes compression ratio over speed. Its slower encode times are intentional, not a deficiency.

## Contributing

Contributions are welcome!

* **New Implementations:** Must implement the standard CLI defined in "Benchmarking Architecture", including `--warmup`, `--threads`, `--verify`, and `--preallocate` flags.
* **Optimization:** If you find flags or methods that improve a specific implementation, open a PR with benchmark results and updated manifest.
* **Image Sets:** Proposals for additional pathological or domain-specific test images are welcome.
