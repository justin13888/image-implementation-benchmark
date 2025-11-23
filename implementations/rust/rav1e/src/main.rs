use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation};
use image::GenericImageView;
use rav1e::prelude::*;
use std::fs;

struct Rav1eBench;

struct BenchContext {
    width: usize,
    height: usize,
    data: Vec<u8>,
}

impl BenchmarkImplementation for Rav1eBench {
    fn name(&self) -> &'static str {
        "rav1e"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        // Rav1e is an encoder. We need raw image data.
        // Assuming input is an image file that we load and convert to YUV or RGB.
        let img = image::open(&args.input).context("Failed to open input image")?;
        let width = img.width() as usize;
        let height = img.height() as usize;
        let data = img.to_rgb8().into_raw();

        Ok(Box::new(BenchContext {
            width,
            height,
            data,
        }))
    }

    fn run(&self, args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");

        let cfg = Config::default();
        let mut ctx_encoder: rav1e::Context<u16> = cfg.new_context().unwrap();

        // Create a frame
        // This is a simplified YUV444 conversion for benchmarking purposes
        let mut frame = ctx_encoder.new_frame();
        for (i, pixel) in ctx.data.chunks(3).enumerate() {
            let y = i / ctx.width;
            let x = i % ctx.width;
            // Just dummy copy to planes to simulate work
            frame.planes[0].data[y * ctx.width + x] = pixel[0] as u16;
            frame.planes[1].data[y * ctx.width + x] = pixel[1] as u16;
            frame.planes[2].data[y * ctx.width + x] = pixel[2] as u16;
        }

        ctx_encoder.send_frame(frame).unwrap();
        ctx_encoder.flush();

        let mut output = Vec::new();
        loop {
            match ctx_encoder.receive_packet() {
                Ok(packet) => {
                    output.extend_from_slice(&packet.data);
                }
                Err(EncoderStatus::Encoded) => break,
                Err(EncoderStatus::LimitReached) => break,
                Err(e) => anyhow::bail!("Encoding error: {:?}", e),
            }
        }

        Ok(output)
    }

    fn verify(&self, _args: &Args, _context: &dyn std::any::Any, _output: &[u8]) -> Result<()> {
        Ok(())
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(Rav1eBench)
}
