use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation};
use std::fs;
use zune_png::PngDecoder;

struct ZunePngBench;

struct BenchContext {
    input_data: Vec<u8>,
}

impl BenchmarkImplementation for ZunePngBench {
    fn name(&self) -> &'static str {
        "zune-png-decode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let input_data = fs::read(&args.input).context("Failed to read input file")?;

        Ok(Box::new(BenchContext { input_data }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");
        let mut decoder = PngDecoder::new(std::io::Cursor::new(&ctx.input_data));

        decoder
            .decode_headers()
            .context("Failed to decode headers")?;
        let (w, h) = decoder
            .dimensions()
            .ok_or_else(|| anyhow::anyhow!("Failed to get dimensions"))?;

        let pixels = decoder.decode().context("Failed to decode PNG")?;

        match pixels {
            zune_png::zune_core::result::DecodingResult::U8(data) => {
                benchmark_harness::encode_ppm_rgb8(w as u32, h as u32, &data)
            }
            zune_png::zune_core::result::DecodingResult::U16(data) => {
                benchmark_harness::encode_ppm_rgb16(w as u32, h as u32, &data)
            }
            _ => anyhow::bail!("Unsupported pixel format"),
        }
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ZunePngBench)
}
