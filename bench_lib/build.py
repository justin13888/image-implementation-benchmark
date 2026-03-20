"""Compilation orchestration for Rust and C++ implementations."""

import os
import shutil
import subprocess
import sys
import threading
from concurrent.futures import ThreadPoolExecutor
from itertools import chain

from colorama import Fore

from bench_lib.models import (
    IMPLEMENTATIONS,
    NULL_IMPLEMENTATIONS,
    ImageFormat,
    Implementation,
    safe_print,
)

# Project root (directory containing bench_lib/)
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VENDOR_DIR = os.path.join(PROJECT_ROOT, "vendor")
VENDOR_COMMON = os.path.join(VENDOR_DIR, "install", "common")
VENDOR_LIBJPEG_TURBO = os.path.join(VENDOR_DIR, "install", "libjpeg-turbo")
VENDOR_MOZJPEG = os.path.join(VENDOR_DIR, "install", "mozjpeg")

# Per-build-directory locks to prevent concurrent cmake/make in the same dir
_build_dir_locks: dict[str, threading.Lock] = {}
_build_dir_locks_lock = threading.Lock()


def _get_build_dir_lock(build_dir: str) -> threading.Lock:
    with _build_dir_locks_lock:
        if build_dir not in _build_dir_locks:
            _build_dir_locks[build_dir] = threading.Lock()
        return _build_dir_locks[build_dir]


def run_build_command(command, cwd, step_name, max_lines=100, env=None):
    """Helper function to run commands with detailed error logging."""
    try:
        subprocess.run(
            command,
            cwd=cwd,
            check=True,
            capture_output=True,
            text=True,
            env=env,
        )
    except subprocess.CalledProcessError as e:
        print(f"\n    ✗ {step_name} failed")
        print(f"    Command: {' '.join(command)}")

        # Helper to print the tail of the log
        def print_log_tail(content, label):
            if content:
                lines = content.strip().split("\n")
                count = len(lines)
                # Get the last `max_lines` lines (where compiler errors usually live)
                tail = lines[-max_lines:]
                print(f"\n    --- {label} (Last {len(tail)} of {count} lines) ---")
                for line in tail:
                    print(f"      {line}")

        print_log_tail(e.stdout, "STDOUT")
        print_log_tail(e.stderr, "STDERR")
        print(f"\n✗ Build process aborted during {step_name}.")
        sys.exit(1)


def build_project(impl: Implementation):
    """Build a project by name."""
    if impl.lang == "rust":
        build_rust_project(impl)
    elif impl.lang == "cpp":
        build_cpp_project(impl)
    else:
        raise ValueError(f"Unknown language: {impl.lang}")


def build_rust_project(impl: Implementation):
    """
    Build a Rust project.

    Assumes the project language is Rust.
    """
    assert impl.lang == "rust", "build_rust_project() called with non-Rust project"

    run_build_command(
        [
            "cargo",
            "build",
            "--release",
        ],  # Note: We don't have a granular way to determine the binary name, so we just build everything
        cwd=".",
        step_name=f"{impl.name} (Cargo Build)",
    )


def _get_vendor_prefix_path(impl_name: str) -> str:
    """Return semicolon-separated CMake prefix path for vendored libs."""
    if "libjpeg-turbo" in impl_name:
        return f"{VENDOR_COMMON};{VENDOR_LIBJPEG_TURBO}"
    if "mozjpeg" in impl_name:
        return f"{VENDOR_COMMON};{VENDOR_MOZJPEG}"
    return VENDOR_COMMON


def _get_pkg_config_path(prefix: str) -> str:
    """Build PKG_CONFIG_PATH from a vendor install prefix."""
    dirs = [
        os.path.join(prefix, "lib", "pkgconfig"),
        os.path.join(prefix, "lib64", "pkgconfig"),
        os.path.join(prefix, "share", "pkgconfig"),
    ]
    existing = os.environ.get("PKG_CONFIG_PATH", "")
    return ":".join(dirs + ([existing] if existing else []))


def build_cpp_project(impl):
    """Build a C++ project with thread-safe logging."""
    assert impl.lang == "cpp", "build_cpp_project() called with non-C++ project"

    bin_path = impl.bin
    build_dir = os.path.dirname(bin_path)

    # Use a prefix so the user can track which thread is doing what
    prefix = f"[{impl.name}]".ljust(15)

    safe_print(f"{Fore.CYAN}{prefix} Starting build...")

    build_lock = _get_build_dir_lock(os.path.abspath(build_dir))
    try:
        with build_lock:
            # If another thread already built this shared directory, skip.
            if os.path.exists(bin_path):
                safe_print(f"{Fore.GREEN}{prefix} ✓ Build complete (shared build dir).")
                return True

            # Remove stale CMakeCache to avoid "different source directory" errors
            # (e.g. after the project moves from /var/data/... to /var/home/...).
            cmake_cache = os.path.join(build_dir, "CMakeCache.txt")
            cmake_files = os.path.join(build_dir, "CMakeFiles")
            if os.path.exists(cmake_cache):
                os.remove(cmake_cache)
            if os.path.exists(cmake_files):
                shutil.rmtree(cmake_files)
            os.makedirs(build_dir, exist_ok=True)

            cmake_prefix = _get_vendor_prefix_path(impl.name)
            pkg_config_path = _get_pkg_config_path(VENDOR_COMMON)

            env = os.environ.copy()
            env["PKG_CONFIG_PATH"] = pkg_config_path

            # 1. Configure with CMake
            run_build_command(
                [
                    "cmake",
                    "..",
                    "-DCMAKE_BUILD_TYPE=Release",
                    "-DCMAKE_C_COMPILER_LAUNCHER=ccache",
                    "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache",
                    f"-DCMAKE_PREFIX_PATH={cmake_prefix}",
                ],
                cwd=build_dir,
                step_name=f"{impl.name} (CMake)",
                env=env,
            )

            # 2. Build with Make
            run_build_command(
                ["make", "-j"],
                cwd=build_dir,
                step_name=f"{impl.name} (Make)",
                env=env,
            )

            # 3. Verify
            if not os.path.exists(bin_path):
                safe_print(f"{Fore.RED}{prefix} ✗ Binary not found: {bin_path}")
                return False
            else:
                safe_print(f"{Fore.GREEN}{prefix} ✓ Build complete.")
                return True

    except Exception as e:
        safe_print(f"{Fore.RED}{prefix} ✗ Build failed: {str(e)}")
        return False


def build_vendor_deps():
    """Build all vendored C/C++ libraries and ssimulacra2."""
    build_vendor_script = os.path.join(VENDOR_DIR, "build_vendor.py")
    try:
        subprocess.run(
            [sys.executable, build_vendor_script],
            check=True,
        )
    except subprocess.CalledProcessError as e:
        print(f"\n✗ Vendor dependency build failed: {e}")
        sys.exit(1)


def build_projects(formats: list[ImageFormat]):
    """Build Rust and C++ projects."""

    print(f"{Fore.BLUE}{'=' * 70}\nBUILDING PROJECTS\n{'=' * 70}")

    # Build vendored C/C++ libraries and ssimulacra2
    print(f"{Fore.BLUE}\n[0/3] Building vendored dependencies...")
    build_vendor_deps()

    # Build Rust projects
    print(f"{Fore.BLUE}\n[1/3] Building Rust projects...")
    try:
        env = os.environ.copy()
        env["RUSTFLAGS"] = "-C target-cpu=native"
        subprocess.run(["cargo", "build", "--release"], check=True, env=env)
        print("  ✓ Rust build complete")
    except subprocess.CalledProcessError as e:
        print(f"  ✗ Rust build failed: {e}")
        sys.exit(1)

    # Build C++ projects
    print(f"{Fore.BLUE}\n[2/3] Building C++ projects...")

    to_build = []
    for fmt in formats:
        for impl in chain(
            NULL_IMPLEMENTATIONS,
            (impl for impl in IMPLEMENTATIONS if impl.format == fmt),
        ):
            if impl.lang == "cpp":
                to_build.append(impl)
    print(f"{Fore.YELLOW}[3/3] Starting 3 simultaneous C++ builds...\n")
    with ThreadPoolExecutor(max_workers=3) as executor:
        results = list(executor.map(build_cpp_project, to_build))

    # Check results
    if all(results):
        print(f"\n{Fore.GREEN}All C++ builds succeeded.")
    else:
        print(f"\n{Fore.RED}Some C++ builds failed.")
        sys.exit(1)

    print("\n✓ Build phase complete\n")
