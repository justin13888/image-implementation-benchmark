use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation};
use image::ImageFormat;
use std::fs;

struct ImagePngBench;

struct BenchContext {
    input_data: Vec<u8>,
}

impl BenchmarkImplementation for ImagePngBench {
    fn name(&self) -> &'static str {
        "image-png-decode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let input_data = fs::read(&args.input).context("Failed to read input file")?;

        Ok(Box::new(BenchContext { input_data }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");
        let img = image::load_from_memory_with_format(&ctx.input_data, ImageFormat::Png)
            .context("Failed to decode PNG")?;

        let rgb = img.to_rgb8();
        benchmark_harness::encode_ppm_rgb8(rgb.width(), rgb.height(), &rgb)
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ImagePngBench)
}
