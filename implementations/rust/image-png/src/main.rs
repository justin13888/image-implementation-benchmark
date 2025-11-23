use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation};
use image::ImageFormat;
use std::fs;

struct ImagePngBench;

struct BenchContext {
    input_data: Vec<u8>,
    reference_pixels: Option<Vec<u8>>,
}

impl BenchmarkImplementation for ImagePngBench {
    fn name(&self) -> &'static str {
        "image-png"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let input_data = fs::read(&args.input).context("Failed to read input file")?;

        let reference_pixels = if args.verify {
            let img = image::load_from_memory_with_format(&input_data, ImageFormat::Png)
                .context("Failed to load reference image")?;
            Some(img.to_rgb8().into_raw())
        } else {
            None
        };

        Ok(Box::new(BenchContext {
            input_data,
            reference_pixels,
        }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");
        let img = image::load_from_memory_with_format(&ctx.input_data, ImageFormat::Png)
            .context("Failed to decode PNG")?;
        Ok(img.to_rgb8().into_raw())
    }

    fn verify(&self, _args: &Args, context: &dyn std::any::Any, output: &[u8]) -> Result<()> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");

        if let Some(ref reference) = ctx.reference_pixels {
            // Lossless check
            if output != reference.as_slice() {
                anyhow::bail!("Output does not match reference (lossless mismatch)");
            }
        } else {
            anyhow::bail!("No reference data available for verification");
        }
        Ok(())
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ImagePngBench)
}
