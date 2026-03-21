"""System information collection for reproducibility manifests."""

import datetime
import os
import platform
import subprocess
from typing import Dict, Any

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VENDOR_COMMON = os.path.join(PROJECT_ROOT, "vendor", "install", "common")
VENDOR_MOZJPEG = os.path.join(PROJECT_ROOT, "vendor", "install", "mozjpeg")


def get_system_info() -> Dict[str, Any]:
    """Collect system information for reproducibility manifest."""
    info = {
        "timestamp": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "os": f"{platform.system()} {platform.release()}",
        "kernel": platform.version(),
        "cpu": "unknown",
        "cores": os.cpu_count() or 0,
    }

    # Try to get CPU info
    try:
        if platform.system() == "Darwin":
            cpu = (
                subprocess.check_output(["sysctl", "-n", "machdep.cpu.brand_string"])
                .decode()
                .strip()
            )
            info["cpu"] = cpu
        elif platform.system() == "Linux":
            with open("/proc/cpuinfo") as f:
                for line in f:
                    if "model name" in line:
                        info["cpu"] = line.split(":")[1].strip()
                        break
    except Exception:
        pass

    return info


def get_compiler_versions() -> Dict[str, str]:
    """Get versions of all compilers used."""
    versions = {}

    try:
        versions["rustc"] = (
            subprocess.check_output(["rustc", "--version"]).decode().strip()
        )
    except Exception:
        versions["rustc"] = "not found"

    try:
        clang_out = subprocess.check_output(
            ["clang", "--version"], stderr=subprocess.STDOUT
        ).decode()
        versions["clang"] = clang_out.split("\n")[0].strip()
    except Exception:
        versions["clang"] = "not found"

    try:
        versions["cmake"] = (
            subprocess.check_output(["cmake", "--version"])
            .decode()
            .split("\n")[0]
            .strip()
        )
    except Exception:
        versions["cmake"] = "not found"

    return versions


def get_library_versions() -> Dict[str, str]:
    """Attempt to determine versions of image libraries."""
    libraries = {}

    # Build PKG_CONFIG_PATH to include vendored install prefix
    pkg_dirs = [
        os.path.join(VENDOR_COMMON, "lib", "pkgconfig"),
        os.path.join(VENDOR_COMMON, "lib64", "pkgconfig"),
        os.path.join(VENDOR_COMMON, "share", "pkgconfig"),
    ]
    existing = os.environ.get("PKG_CONFIG_PATH", "")
    env = os.environ.copy()
    env["PKG_CONFIG_PATH"] = ":".join(pkg_dirs + ([existing] if existing else []))

    libs_to_check = [
        "libjpeg-turbo",
        "libpng",
        "libwebp",
        "libavif",
        "dav1d",
        "libjxl",
    ]

    for lib in libs_to_check:
        try:
            result = (
                subprocess.check_output(
                    ["pkg-config", "--modversion", lib],
                    stderr=subprocess.DEVNULL,
                    env=env,
                )
                .decode()
                .strip()
            )
            libraries[lib] = result
        except Exception:
            libraries[lib] = "unknown"

    # Detect mozjpeg version from vendored .pc file
    mozjpeg_pc = os.path.join(VENDOR_MOZJPEG, "lib64", "pkgconfig", "libjpeg.pc")
    if not os.path.exists(mozjpeg_pc):
        mozjpeg_pc = os.path.join(VENDOR_MOZJPEG, "lib", "pkgconfig", "libjpeg.pc")
    mozjpeg_version = "unknown"
    if os.path.exists(mozjpeg_pc):
        try:
            mozjpeg_env = os.environ.copy()
            mozjpeg_env["PKG_CONFIG_PATH"] = os.path.dirname(mozjpeg_pc)
            mozjpeg_version = (
                subprocess.check_output(
                    ["pkg-config", "--modversion", "libjpeg"],
                    stderr=subprocess.DEVNULL,
                    env=mozjpeg_env,
                )
                .decode()
                .strip()
            )
        except Exception:
            pass
    libraries["mozjpeg"] = mozjpeg_version
    libraries["mimalloc"] = "2.1.7"  # Pinned in vendor/mimalloc submodule
    libraries["hyperfine"] = "unknown"

    try:
        hf_version = (
            subprocess.check_output(["hyperfine", "--version"]).decode().strip()
        )
        libraries["hyperfine"] = (
            hf_version.split()[1] if len(hf_version.split()) > 1 else hf_version
        )
    except Exception:
        pass

    return libraries
