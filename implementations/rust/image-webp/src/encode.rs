use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation};
use std::io::{BufWriter, Cursor};

struct ImageWebpBench;

struct BenchContext {
    img: image::DynamicImage,
}

impl BenchmarkImplementation for ImageWebpBench {
    fn name(&self) -> &'static str {
        "image-webp-encode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let (width, height, rgb_data) = benchmark_harness::decode_ppm_rgb8(&args.input)?;
        let img = image::RgbImage::from_raw(width, height, rgb_data)
            .context("Failed to create RgbImage")?;
        let img = image::DynamicImage::ImageRgb8(img);
        Ok(Box::new(BenchContext { img }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");

        let mut output = Vec::with_capacity(ctx.img.as_bytes().len() / 2);

        {
            let cursor = Cursor::new(&mut output);
            let mut writer = BufWriter::new(cursor);
            // TODO: image-webp crate only supports lossless encoding as of writing.
            // This means quality tiers (web-low, web-high) are not respected.
            ctx.img
                .write_to(&mut writer, image::ImageFormat::WebP)
                .context("Failed to encode WebP")?;
        }

        Ok(output)
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ImageWebpBench)
}
