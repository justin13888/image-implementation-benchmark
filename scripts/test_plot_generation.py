#!/usr/bin/env python3
"""
Sanity test for plot generation in bench.generate_summary.

This script constructs a small hyperfine-like JSON file and calls
bench.generate_summary to verify the creation of separate plot files.
"""

import os
import sys
import json
import types

# If we're not running in a virtualenv, attempt to re-exec using `.venv/bin/python`
if not os.environ.get("VIRTUAL_ENV"):
    venv_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".venv"))
    candidate = os.path.join(venv_root, "bin", "python")
    if os.path.exists(candidate):
        print(f"Re-executing using venv python: {candidate}")
        # Ensure the new process inherits a VIRTUAL_ENV env var and updated PATH
        os.environ["VIRTUAL_ENV"] = venv_root
        os.environ["PATH"] = (
            os.path.join(venv_root, "bin") + ":" + os.environ.get("PATH", "")
        )
        os.execv(candidate, [candidate] + sys.argv)
    else:
        print(
            "Warning: Not running in a virtualenv. If you want tests to use a venv, create one at .venv and re-run the script."
        )

# Load `bench` script (file without .py extension) as a module by reading its
# source and executing it in a fresh module namespace.
bench_path = os.path.join(os.path.dirname(__file__), "..", "bench")
bench_path = os.path.abspath(bench_path)
bench = types.ModuleType("bench")
with open(bench_path, "r") as bf:
    source = bf.read()
exec(compile(source, bench_path, "exec"), bench.__dict__)
sys.modules["bench"] = bench
from bench import generate_summary  # noqa: E402


def main():
    # Minimal hyperfine JSON structure
    data = {
        "results": [
            {
                "command": "implA (jpeg, decode, file.jpg)",
                "mean": 0.05,
                "stddev": 0.005,
                "min": 0.04,
                "max": 0.06,
                "times": [0.05, 0.052, 0.048],
            },
            {
                "command": "implB (jpeg, decode, file.jpg)",
                "mean": 0.08,
                "stddev": 0.008,
                "min": 0.07,
                "max": 0.09,
                "times": [0.08, 0.081, 0.079],
            },
        ]
    }

    tmpdir = os.path.join(os.path.dirname(__file__), "../results/tmp")
    os.makedirs(tmpdir, exist_ok=True)
    raw_json_path = os.path.join(tmpdir, "raw.json")
    with open(raw_json_path, "w") as f:
        json.dump(data, f)

    # Call generate_summary which should create PNGs and summary.md
    generate_summary(tmpdir, raw_json_path)

    # Check result files
    print("Directory contents:")
    files = sorted(os.listdir(tmpdir))
    for fname in files:
        print(" -", fname)

    # Ensure at least one PNG (plot) and the summary exist
    pngs = [f for f in files if f.endswith(".png")]
    assert len(pngs) >= 1, "Expected at least one plot PNG to be generated"

    assert "summary.md" in files, "Expected summary.md to be generated"


if __name__ == "__main__":
    main()
