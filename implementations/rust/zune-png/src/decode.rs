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

        use std::io::Write;

        match pixels {
            zune_png::zune_core::result::DecodingResult::U8(data) => {
                let mut output = Vec::with_capacity(20 + data.len());
                write!(&mut output, "P6\n{} {}\n255\n", w, h)?;
                output.write_all(&data)?;
                Ok(output)
            }
            zune_png::zune_core::result::DecodingResult::U16(data) => {
                let mut output = Vec::with_capacity(20 + data.len() * 2);
                write!(&mut output, "P6\n{} {}\n65535\n", w, h)?;
                for val in data {
                    output.write_all(&val.to_be_bytes())?;
                }
                Ok(output)
            }
            _ => anyhow::bail!("Unsupported pixel format"),
        }
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ZunePngBench)
}
