"""Compilation orchestration for Rust and C++ implementations."""

import os
import subprocess
import sys
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


def run_build_command(command, cwd, step_name, max_lines=100):
    """Helper function to run commands with detailed error logging."""
    try:
        subprocess.run(
            command,
            cwd=cwd,
            check=True,
            capture_output=True,
            text=True,
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


def build_cpp_project(impl):
    """Build a C++ project with thread-safe logging."""
    assert impl.lang == "cpp", "build_cpp_project() called with non-C++ project"

    bin_path = impl.bin
    build_dir = os.path.dirname(bin_path)

    # Use a prefix so the user can track which thread is doing what
    prefix = f"[{impl.name}]".ljust(15)

    safe_print(f"{Fore.CYAN}{prefix} Starting build...")

    try:
        os.makedirs(build_dir, exist_ok=True)

        # 1. Configure with CMake
        run_build_command(
            [
                "cmake",
                "..",
                "-DCMAKE_BUILD_TYPE=Release",
                "-DCMAKE_C_COMPILER_LAUNCHER=ccache",
                "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache",
            ],
            cwd=build_dir,
            step_name=f"{impl.name} (CMake)",
        )

        # 2. Build with Make
        run_build_command(
            ["make", "-j"],
            cwd=build_dir,
            step_name=f"{impl.name} (Make)",
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


def build_projects(formats: list[ImageFormat]):
    """Build Rust and C++ projects."""

    print(f"{Fore.BLUE}{'=' * 70}\nBUILDING PROJECTS\n{'=' * 70}")

    # Build Rust projects
    print(f"{Fore.BLUE}\n[1/2] Building Rust projects...")
    try:
        env = os.environ.copy()
        env["RUSTFLAGS"] = "-C target-cpu=native"
        subprocess.run(["cargo", "build", "--release"], check=True, env=env)
        print("  ✓ Rust build complete")
    except subprocess.CalledProcessError as e:
        print(f"  ✗ Rust build failed: {e}")
        sys.exit(1)

    # Build C++ projects
    print(f"{Fore.BLUE}\n[2/2] Building C++ projects...")

    to_build = []
    for fmt in formats:
        for impl in chain(
            NULL_IMPLEMENTATIONS,
            (impl for impl in IMPLEMENTATIONS if impl.format == fmt),
        ):
            if impl.lang == "cpp":
                to_build.append(impl)
    print(f"{Fore.YELLOW}Starting 3 simultaneous C++ builds...\n")
    with ThreadPoolExecutor(max_workers=3) as executor:
        results = list(executor.map(build_cpp_project, to_build))

    # Check results
    if all(results):
        print(f"\n{Fore.GREEN}All C++ builds succeeded.")
    else:
        print(f"\n{Fore.RED}Some C++ builds failed.")
        sys.exit(1)

    print("\n✓ Build phase complete\n")
