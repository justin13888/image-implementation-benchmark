use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation, Quality};
use image::codecs::jpeg::JpegEncoder;
use image::ImageEncoder;

struct ImageJpegBench;

struct BenchContext {
    rgb_data: Vec<u8>,
    width: u32,
    height: u32,
    quality: u8,
}

impl BenchmarkImplementation for ImageJpegBench {
    fn name(&self) -> &'static str {
        "image-jpeg-encode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let (width, height, rgb_data) = benchmark_harness::decode_ppm_rgb8(&args.input)?;

        // Map quality to JPEG quality (1-100)
        let quality = match args.quality {
            Quality::WebLow => 50,
            Quality::WebHigh => 80,
            Quality::Archival => 95,
        };

        Ok(Box::new(BenchContext {
            rgb_data,
            width,
            height,
            quality,
        }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");

        let mut output = Vec::with_capacity(ctx.rgb_data.len() / 2);
        {
            let encoder = JpegEncoder::new_with_quality(&mut output, ctx.quality);
            encoder
                .write_image(
                    &ctx.rgb_data,
                    ctx.width,
                    ctx.height,
                    image::ColorType::Rgb8.into(),
                )
                .context("Failed to encode JPEG")?;
        }

        Ok(output)
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ImageJpegBench)
}
