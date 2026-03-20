use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation};
use image::codecs::webp::WebPEncoder;
use image::{ExtendedColorType, ImageEncoder};
use std::io::{BufWriter, Cursor};

struct ImageWebpBench;

struct BenchContext {
    width: u32,
    height: u32,
    rgb_data: Vec<u8>,
}

impl BenchmarkImplementation for ImageWebpBench {
    fn name(&self) -> &'static str {
        "image-webp-encode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let (width, height, rgb_data) = benchmark_harness::decode_ppm_rgb8(&args.input)?;
        Ok(Box::new(BenchContext {
            width,
            height,
            rgb_data,
        }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");

        let mut output = Vec::with_capacity(ctx.rgb_data.len() / 2);

        {
            let cursor = Cursor::new(&mut output);
            let writer = BufWriter::new(cursor);
            // LIMITATION: image-webp v0.2.4 only supports lossless WebP encoding.
            // Quality tiers (web-low, web-high) are NOT respected — all output is lossless.
            // This makes timing comparisons with libwebp-encode for lossy tiers invalid.
            // Exclude image-webp from lossy-tier comparisons until a lossy API is available.
            WebPEncoder::new_lossless(writer)
                .write_image(
                    &ctx.rgb_data,
                    ctx.width,
                    ctx.height,
                    ExtendedColorType::Rgb8,
                )
                .context("Failed to encode WebP (lossless)")?;
        }

        Ok(output)
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ImageWebpBench)
}
