use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation};
use image::DynamicImage;
use jxl_oxide::integration::JxlDecoder;
use std::fs;
use std::io::Cursor;

struct JxlOxideBench;

struct BenchContext {
    input_data: Vec<u8>,
}

impl BenchmarkImplementation for JxlOxideBench {
    fn name(&self) -> &'static str {
        "jxl-oxide-decode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let input_data = fs::read(&args.input).context("Failed to read input file")?;

        Ok(Box::new(BenchContext { input_data }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");
        let decoder = JxlDecoder::new(Cursor::new(&ctx.input_data))?;
        let img = DynamicImage::from_decoder(decoder)?;

        // Note we skip obtaining ICC profile and applying colour transform.

        // Output as PPM
        let rgb = img.to_rgb8();
        let mut output = Vec::with_capacity(15 + rgb.len()); // Header + Data
        use std::io::Write;
        write!(&mut output, "P6\n{} {}\n255\n", rgb.width(), rgb.height())?;
        output.write_all(&rgb)?;

        Ok(output)
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(JxlOxideBench)
}
