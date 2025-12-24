use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation};
use std::fs;
use zune_jpeg::JpegDecoder;

struct ZuneJpegBench;

struct BenchContext {
    input_data: Vec<u8>,
}

impl BenchmarkImplementation for ZuneJpegBench {
    fn name(&self) -> &'static str {
        "zune-jpeg-decode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let input_data = fs::read(&args.input).context("Failed to read input file")?;

        Ok(Box::new(BenchContext { input_data }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");
        let mut decoder = JpegDecoder::new(std::io::Cursor::new(&ctx.input_data));
        decoder
            .decode_headers()
            .context("Failed to decode headers")?;
        let (w, h) = decoder
            .dimensions()
            .ok_or_else(|| anyhow::anyhow!("Failed to get dimensions"))?;
        let pixels = decoder.decode().context("Failed to decode JPEG")?;

        // Output as PPM
        let mut output = Vec::with_capacity(20 + pixels.len());
        use std::io::Write;
        write!(&mut output, "P6\n{} {}\n255\n", w, h)?;
        output.write_all(&pixels)?;
        Ok(output)
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ZuneJpegBench)
}
