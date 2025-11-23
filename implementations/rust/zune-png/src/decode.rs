use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation};
use std::fs;
use zune_png::PngDecoder;

struct ZunePngBench;

struct BenchContext {
    input_data: Vec<u8>,
    reference_pixels: Option<Vec<u8>>,
}

impl BenchmarkImplementation for ZunePngBench {
    fn name(&self) -> &'static str {
        "zune-png-decode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let input_data = fs::read(&args.input).context("Failed to read input file")?;

        let reference_pixels = if args.verify {
            let img =
                image::load_from_memory(&input_data).context("Failed to load reference image")?;
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
        let mut decoder = PngDecoder::new(std::io::Cursor::new(&ctx.input_data));
        let pixels = decoder.decode().context("Failed to decode PNG")?;

        // zune-png returns DecodingResult
        match pixels {
            zune_png::zune_core::result::DecodingResult::U8(data) => Ok(data),
            zune_png::zune_core::result::DecodingResult::U16(data) => {
                // Convert u16 to u8 (strip LSB or just cast) for benchmarking purposes
                // Or just return as bytes
                let mut bytes = Vec::with_capacity(data.len() * 2);
                for val in data {
                    bytes.extend_from_slice(&val.to_ne_bytes());
                }
                Ok(bytes)
            }
            _ => anyhow::bail!("Unsupported pixel format"),
        }
    }

    fn verify(&self, _args: &Args, context: &dyn std::any::Any, output: &[u8]) -> Result<()> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");

        if let Some(ref reference) = ctx.reference_pixels {
            // Lossless check
            if output != reference.as_slice() {
                // If size mismatch, it's definitely wrong.
                if output.len() != reference.len() {
                    anyhow::bail!(
                        "Output size mismatch: {} vs {}",
                        output.len(),
                        reference.len()
                    );
                }

                // If bytes mismatch, it could be RGB vs RGBA or different layout.
                // But assuming standard PNG, it should match.
                anyhow::bail!("Output does not match reference (lossless mismatch)");
            }
        } else {
            anyhow::bail!("No reference data available for verification");
        }
        Ok(())
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ZunePngBench)
}
