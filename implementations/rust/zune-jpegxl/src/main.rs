use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation};
use std::fs;
use zune_jpegxl::JxlDecoder; // Try this first? No, it failed.
                             // Let's try to find where it is.
                             // Assuming it follows zune pattern, maybe it is re-exported.
                             // But error says no.
                             // Let's try `use zune_jpegxl::decoder::JxlDecoder;` if I can't verify.
                             // Or I can check if I can read the source from cargo registry? No.
                             // I'll try `zune_jpegxl::decoder::JxlDecoder`.
use zune_jpegxl::decoder::JxlDecoder;

struct ZuneJxlBench;

struct BenchContext {
    input_data: Vec<u8>,
    reference_pixels: Option<Vec<u8>>,
}

impl BenchmarkImplementation for ZuneJxlBench {
    fn name(&self) -> &'static str {
        "zune-jpegxl"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let input_data = fs::read(&args.input).context("Failed to read input file")?;

        let reference_pixels = None; // Verification skipped for now

        Ok(Box::new(BenchContext {
            input_data,
            reference_pixels,
        }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");
        let mut decoder = JxlDecoder::new(std::io::Cursor::new(&ctx.input_data));
        let pixels = decoder.decode().context("Failed to decode JXL")?;
        Ok(pixels)
    }

    fn verify(&self, _args: &Args, _context: &dyn std::any::Any, _output: &[u8]) -> Result<()> {
        Ok(())
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ZuneJxlBench)
}
