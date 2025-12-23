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
        let pixels = decoder.decode().context("Failed to decode PNG")?;

        match pixels {
            zune_png::zune_core::result::DecodingResult::U8(data) => Ok(data),
            zune_png::zune_core::result::DecodingResult::U16(data) => {
                // TODO: Check if there's performance issues with this conversion.
                let mut bytes = Vec::with_capacity(data.len() * 2);
                for val in data {
                    bytes.extend_from_slice(&val.to_ne_bytes());
                }
                Ok(bytes)
            }
            _ => anyhow::bail!("Unsupported pixel format"),
        }
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ZunePngBench)
}
