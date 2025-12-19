use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation};
use jpeg_decoder::Decoder;
use std::fs;
use std::io::Cursor;

struct JpegDecoderBench;

struct BenchContext {
    input_data: Vec<u8>,
    reference_pixels: Option<Vec<u8>>,
}

impl BenchmarkImplementation for JpegDecoderBench {
    fn name(&self) -> &'static str {
        "jpeg-decoder-decode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let input_data = fs::read(&args.input).context("Failed to read input file")?;

        let reference_pixels = if args.verify {
            // Use image crate as reference
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
        let cursor = Cursor::new(&ctx.input_data);
        let mut decoder = Decoder::new(cursor);
        let pixels = decoder.decode().context("Failed to decode JPEG")?;
        Ok(pixels)
    }

    fn verify(&self, args: &Args, context: &dyn std::any::Any, output: &[u8]) -> Result<()> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");

        if let Some(ref reference) = ctx.reference_pixels {
            let psnr = benchmark_harness::calculate_psnr(output, reference)?;
            if psnr < args.verify_threshold {
                anyhow::bail!(
                    "PSNR too low: {psnr:.2} dB (threshold: {:.2} dB)",
                    args.verify_threshold
                );
            }
            // println!("Verification passed: PSNR = {:.2} dB", psnr);
        } else {
            anyhow::bail!("No reference data available for verification");
        }
        Ok(())
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(JpegDecoderBench)
}
