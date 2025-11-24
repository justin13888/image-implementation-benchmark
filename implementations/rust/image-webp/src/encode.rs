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
        let img = image::open(&args.input).context("Failed to open input image")?;
        let img = image::DynamicImage::ImageRgb8(img.to_rgb8());
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
            // image-webp crate (used by image 0.24) is currently lossless only.
            // We cannot set quality.
            ctx.img
                .write_to(&mut writer, image::ImageFormat::WebP)
                .context("Failed to encode WebP")?;
        }

        Ok(output)
    }

    fn verify(&self, _args: &Args, _context: &dyn std::any::Any, output: &[u8]) -> Result<()> {
        if output.is_empty() {
            anyhow::bail!("Encoder produced empty output");
        }
        // Check WebP signature (RIFF ... WEBP)
        if output.len() < 12 || &output[0..4] != b"RIFF" || &output[8..12] != b"WEBP" {
            anyhow::bail!("Output is not a valid WebP");
        }
        Ok(())
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ImageWebpBench)
}
