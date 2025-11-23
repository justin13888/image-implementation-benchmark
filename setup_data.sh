#!/bin/bash
set -e
mkdir -p data

echo "Generating test images..."

# Generate a random noise image (good for stress testing)
convert -size 1024x1024 xc: +noise Random data/test.ppm
convert data/test.ppm -quality 80 data/test.jpg
convert data/test.ppm data/test.png
convert data/test.ppm -quality 80 data/test.webp
# ImageMagick might not support avif/jxl depending on version, but let's try or use fallbacks if needed.
# If convert fails, we might need another tool or just skip.
convert data/test.ppm -quality 80 data/test.avif || echo "Warning: AVIF generation failed"

# Use cjxl for JXL encoding (more reliable than ImageMagick)
if command -v cjxl &> /dev/null; then
    cjxl data/test.ppm data/test.jxl -q 80
else
    echo "Warning: cjxl not found, JXL generation skipped"
fi

echo "Done."
