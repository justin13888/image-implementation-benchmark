#!/usr/bin/env python3
"""
Select 20 diverse images from DIV2K dataset using perceptual hash distance.
Maximizes visual diversity for better benchmark coverage.
"""
import os
import sys
from pathlib import Path
try:
    from PIL import Image
    import imagehash
    import numpy as np
except ImportError:
    print("Error: Required packages not found. Install with:")
    print("  pip install pillow imagehash numpy")
    sys.exit(1)

def select_diverse_images(image_dir, output_list, n=20):
    """Select n diverse images using greedy farthest-first selection."""
    images = list(Path(image_dir).glob("*.png"))
    if len(images) < n:
        print(f"Warning: Only {len(images)} images found, selecting all")
        n = len(images)
    
    # Compute perceptual hashes
    print(f"Computing hashes for {len(images)} images...")
    hashes = {}
    for img_path in images:
        try:
            img = Image.open(img_path)
            hashes[img_path] = imagehash.phash(img, hash_size=16)
        except Exception as e:
            print(f"Warning: Failed to hash {img_path}: {e}")
    
    if len(hashes) < n:
        n = len(hashes)
    
    # Greedy selection: pick first arbitrarily, then pick images farthest from selected set
    selected = []
    remaining = list(hashes.keys())
    
    # Start with first image
    selected.append(remaining.pop(0))
    
    # Greedily select remaining
    while len(selected) < n and remaining:
        max_dist = -1
        best_idx = 0
        
        for i, candidate in enumerate(remaining):
            # Compute min distance to any selected image
            min_dist = min(hashes[candidate] - hashes[s] for s in selected)
            if min_dist > max_dist:
                max_dist = min_dist
                best_idx = i
        
        selected.append(remaining.pop(best_idx))
        print(f"  Selected {len(selected)}/{n}: {selected[-1].name} (diversity score: {max_dist})")
    
    # Write output
    with open(output_list, 'w') as f:
        for img in selected:
            f.write(f"{img.name}\n")
    
    print(f"\n✓ Selected {len(selected)} diverse images, saved to {output_list}")

if __name__ == "__main__":
    select_diverse_images("data/div2k/DIV2K_train_HR", "data/div2k/selected.txt")
