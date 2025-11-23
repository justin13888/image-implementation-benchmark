#!/bin/bash
set -e
mkdir -p data data/kodak data/div2k data/pathological

echo "=== Image Implementation Benchmark Data Setup ==="
echo

# ============================================================================
# Dependency Checks
# ============================================================================
echo "Checking dependencies..."
MISSING_DEPS=()

command -v wget &> /dev/null || MISSING_DEPS+=("wget")
command -v unzip &> /dev/null || MISSING_DEPS+=("unzip")
command -v convert &> /dev/null || MISSING_DEPS+=("imagemagick (convert)")
command -v uv &> /dev/null || MISSING_DEPS+=("uv")

if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
    echo "Error: Missing required dependencies:"
    for dep in "${MISSING_DEPS[@]}"; do
        echo "  - $dep"
    done
    echo
    echo "Install with:"
    echo "  sudo apt install wget unzip imagemagick  # Debian/Ubuntu"
    echo "  curl -LsSf https://astral.sh/uv/install.sh | sh  # uv"
    exit 1
fi
echo "  ✓ All required dependencies found"
echo

# ============================================================================
# 1. KODAK Dataset (Standard - L2/L3 Cache Resident)
# ============================================================================
echo "[1/4] Downloading KODAK dataset..."
if [ ! -d "data/kodak" ] || [ -z "$(ls -A data/kodak 2>/dev/null)" ]; then
    mkdir -p data/kodak
    for i in $(seq -f "%02g" 1 24); do
        if [ ! -f "data/kodak/kodim${i}.png" ]; then
            wget -q "http://r0k.us/graphics/kodak/kodak/kodim${i}.png" -O "data/kodak/kodim${i}.png" || {
                echo "Error: Failed to download kodim${i}.png"
                exit 1
            }
        fi
    done
    echo "  ✓ KODAK dataset ready (24 images)"
else
    echo "  ✓ KODAK dataset already exists"
fi

# ============================================================================
# 2. DIV2K Dataset (High-Res - Memory Bound)
# ============================================================================
echo "[2/4] Setting up DIV2K dataset..."
if [ ! -f "data/div2k/DIV2K_train_HR.zip" ]; then
    echo "  Downloading DIV2K training set (takes a while, ~3.5GB)..."
    mkdir -p data/div2k
    wget -q --show-progress "http://data.vision.ee.ethz.ch/cvl/DIV2K/DIV2K_train_HR.zip" -O "data/div2k/DIV2K_train_HR.zip" || {
        echo "Error: Failed to download DIV2K dataset"
        exit 1
    }
    echo "  ✓ DIV2K downloaded"
fi

if [ ! -d "data/div2k/DIV2K_train_HR" ]; then
    unzip -q data/div2k/DIV2K_train_HR.zip -d data/div2k/ || {
        echo "Error: Failed to extract DIV2K dataset"
        exit 1
    }
    echo "  ✓ DIV2K extracted"
else
    echo "  ✓ DIV2K already extracted"
fi

# Select diverse images using the selection script
if [ ! -f "scripts/select_div2k.py" ]; then
    echo "  Creating DIV2K selection script..."
    cat > scripts/select_div2k.py << 'EOF'
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
    print("  uv sync")
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
EOF
    chmod +x scripts/select_div2k.py
fi

if [ ! -f "data/div2k/selected.txt" ]; then
    echo "  Selecting 20 diverse images..."
    uv run scripts/select_div2k.py
else
    echo "  ✓ DIV2K selection already exists"
fi

# ============================================================================
# 3. Pathological Test Cases
# ============================================================================
echo "[3/4] Generating pathological test cases..."

# Solid color (4K)
if [ ! -f "data/pathological/solid_4k.png" ]; then
    convert -size 3840x2160 xc:"#4287f5" data/pathological/solid_4k.png
    echo "  ✓ solid_4k.png (tests RLE/skip optimizations)"
else
    echo "  ✓ solid_4k.png already exists"
fi

# Gaussian noise (4K)
if [ ! -f "data/pathological/noise_4k.png" ]; then
    convert -size 3840x2160 xc: +noise Gaussian data/pathological/noise_4k.png
    echo "  ✓ noise_4k.png (worst-case for compressors)"
else
    echo "  ✓ noise_4k.png already exists"
fi

# Screenshot with text and flat regions (4K)
if [ ! -f "data/pathological/screenshot_4k.png" ]; then
    convert -size 3840x2160 xc:white \
        -fill "#2d2d2d" -draw "rectangle 0,0 3840,100" \
        -fill "#f5f5f5" -draw "rectangle 0,100 800,2160" \
        -fill white -draw "rectangle 800,100 3840,2160" \
        -fill black -pointsize 72 -annotate +100+60 "File Edit View Help" \
        -fill "#666666" -pointsize 48 -annotate +850+500 "Lorem ipsum dolor sit amet" \
        -fill black -pointsize 36 -annotate +850+600 "const foo = 'bar';" \
        data/pathological/screenshot_4k.png
    echo "  ✓ screenshot_4k.png (UI screenshot simulation)"
else
    echo "  ✓ screenshot_4k.png already exists"
fi

# Alpha gradient (4K)
if [ ! -f "data/pathological/alpha_gradient_4k.png" ]; then
    convert -size 3840x2160 gradient: \
        -size 3840x2160 gradient:"rgba(255,0,0,0)-rgba(0,0,255,255)" \
        -compose over -composite \
        data/pathological/alpha_gradient_4k.png
    echo "  ✓ alpha_gradient_4k.png (transparency gradient)"
else
    echo "  ✓ alpha_gradient_4k.png already exists"
fi

# ============================================================================
# 4. Reference Encodings (for decode benchmarks)
# ============================================================================
echo "[4/4] Generating reference encodings..."

# Generate basic test image if not exists
if [ ! -f "data/test.ppm" ]; then
    convert -size 1024x1024 xc: +noise Random data/test.ppm
fi

# JPEG encodings (using ImageMagick - always available)
if [ ! -f "data/test_web-low.jpg" ]; then
    convert data/test.ppm -quality 50 -sampling-factor 4:2:0 data/test_web-low.jpg
fi
if [ ! -f "data/test_web-high.jpg" ]; then
    convert data/test.ppm -quality 80 -interlace Plane data/test_web-high.jpg
fi
if [ ! -f "data/test_archival.jpg" ]; then
    convert data/test.ppm -quality 95 -sampling-factor 4:4:4 data/test_archival.jpg
fi
echo "  ✓ JPEG encodings generated"

# PNG encoding (using ImageMagick - always available)
if [ ! -f "data/test.png" ]; then
    convert data/test.ppm data/test.png
fi
echo "  ✓ PNG encoding generated"

# Optional encodings - warn but don't fail if tools missing
OPTIONAL_TOOLS=()
command -v cwebp &> /dev/null || OPTIONAL_TOOLS+=("cwebp (WebP)")
command -v avifenc &> /dev/null || OPTIONAL_TOOLS+=("avifenc (AVIF)")
command -v cjxl &> /dev/null || OPTIONAL_TOOLS+=("cjxl (JPEG XL)")

if [ ${#OPTIONAL_TOOLS[@]} -ne 0 ]; then
    echo "  Note: Optional encoding tools not found (benchmarks for these formats will be limited):"
    for tool in "${OPTIONAL_TOOLS[@]}"; do
        echo "    - $tool"
    done
fi

# WebP encodings (optional)
if command -v cwebp &> /dev/null; then
    [ ! -f "data/test_web-low.webp" ] && cwebp -q 50 -m 4 data/test.ppm -o data/test_web-low.webp 2>/dev/null
    [ ! -f "data/test_web-high.webp" ] && cwebp -q 75 -m 4 data/test.ppm -o data/test_web-high.webp 2>/dev/null
    [ ! -f "data/test_archival.webp" ] && cwebp -lossless -z 6 data/test.ppm -o data/test_archival.webp 2>/dev/null
    echo "  ✓ WebP encodings generated"
fi

# AVIF encodings (optional)
if command -v avifenc &> /dev/null; then
    # Convert PPM to PNG first since avifenc works better with PNG
    if [ ! -f "data/test.png" ]; then
        convert data/test.ppm data/test.png
    fi
    [ ! -f "data/test_web-low.avif" ] && avifenc -q 65 -s 6 data/test.png data/test_web-low.avif 2>/dev/null
    [ ! -f "data/test_web-high.avif" ] && avifenc -q 65 data/test.png data/test_web-high.avif 2>/dev/null
    [ ! -f "data/test_archival.avif" ] && avifenc -q 85 -y 444 data/test.png data/test_archival.avif 2>/dev/null
    echo "  ✓ AVIF encodings generated"
fi

# JXL encodings (optional)
if command -v cjxl &> /dev/null; then
    [ ! -f "data/test_web-low.jxl" ] && cjxl data/test.ppm data/test_web-low.jxl -d 4.0 -e 7 2>/dev/null
    [ ! -f "data/test_web-high.jxl" ] && cjxl data/test.ppm data/test_web-high.jxl -d 1.0 -e 7 2>/dev/null
    [ ! -f "data/test_archival.jxl" ] && cjxl data/test.ppm data/test_archival.jxl -d 0 2>/dev/null
    echo "  ✓ JXL encodings generated"
fi

echo
echo "✓ Setup complete! Core datasets ready:"
echo "  - data/kodak/          : 24 KODAK images (~0.4MP)"
echo "  - data/div2k/          : DIV2K dataset + 20 selected (2K/4K)"
echo "  - data/pathological/   : 4 synthetic stress tests"
echo "  - data/test*.{jpg,png} : JPEG and PNG reference encodings"
if [ ${#OPTIONAL_TOOLS[@]} -ne 0 ]; then
    echo
    echo "Note: To benchmark all formats, install optional tools:"
    for tool in "${OPTIONAL_TOOLS[@]}"; do
        echo "  - $tool"
    done
fi
