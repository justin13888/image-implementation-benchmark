#!/usr/bin/env python3
"""
Select 20 diverse images from DIV2K dataset using perceptual hash distance.
Maximizes visual diversity for better benchmark coverage.

This script delegates to bench_lib.data_setup._select_diverse_images, which
is the canonical implementation. Run from the project root with `uv run`.
"""

import sys
from pathlib import Path

# Ensure project root is on sys.path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from bench_lib.data_setup import _select_diverse_images  # noqa: E402


def main():
    image_dir = Path("data/div2k/DIV2K_train_HR")
    output_list = Path("data/div2k/selected.txt")

    if not image_dir.exists():
        print(f"Error: {image_dir} not found. Run './bench setup -d div2k' first.")
        sys.exit(1)

    selected = _select_diverse_images(image_dir, n=20)

    output_list.parent.mkdir(parents=True, exist_ok=True)
    with open(output_list, "w") as f:
        for name in selected:
            f.write(f"{name}\n")

    print(f"\n✓ Selected {len(selected)} diverse images, saved to {output_list}")


if __name__ == "__main__":
    main()
