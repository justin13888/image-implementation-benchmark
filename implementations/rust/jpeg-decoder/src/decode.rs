use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation};
use jpeg_decoder::Decoder;
use std::fs;
use std::io::Cursor;

struct JpegDecoderBench;

struct BenchContext {
    input_data: Vec<u8>,
}

impl BenchmarkImplementation for JpegDecoderBench {
    fn name(&self) -> &'static str {
        "jpeg-decoder-decode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let input_data = fs::read(&args.input).context("Failed to read input file")?;

        Ok(Box::new(BenchContext { input_data }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");
        let cursor = Cursor::new(&ctx.input_data);
        let mut decoder = Decoder::new(cursor);
        let pixels = decoder.decode().context("Failed to decode JPEG")?;
        let info = decoder.info().context("Failed to get image info")?;

        // jpeg_decoder typically outputs RGB for standard JPEGs.
        // We assume RGB (PixelFormat::RGB24).
        benchmark_harness::encode_ppm_rgb8(info.width as u32, info.height as u32, &pixels)
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(JpegDecoderBench)
}
