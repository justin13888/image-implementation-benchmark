use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation, Quality};
use jpeg_encoder::{ColorType, Encoder};

struct JpegEncoderBench;

struct BenchContext {
    quality: u8,
    is_progressive: bool,
    width: u16,
    height: u16,
    rgb8_img: Vec<u8>,
}

impl BenchmarkImplementation for JpegEncoderBench {
    fn name(&self) -> &'static str {
        "jpeg-encoder-encode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let input_data = std::fs::read(&args.input).context("Failed to read input file")?;
        let img = image::load_from_memory_with_format(&input_data, image::ImageFormat::Pnm)
            .context("Failed to decode input PPM")?;
        let quality = match args.quality {
            Quality::WebLow => 50,
            Quality::WebHigh => 80,
            Quality::Archival => 95,
        };
        let is_progressive = match args.quality {
            Quality::WebLow => false,
            Quality::WebHigh => true,
            Quality::Archival => false,
        };
        Ok(Box::new(BenchContext {
            quality,
            is_progressive,
            width: img.width() as u16,
            height: img.height() as u16,
            rgb8_img: img.to_rgb8().to_vec(),
        }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");

        let mut output = Vec::with_capacity(ctx.rgb8_img.len());
        let mut encoder = Encoder::new(&mut output, ctx.quality);
        encoder.set_progressive(ctx.is_progressive);
        encoder
            .encode(&ctx.rgb8_img, ctx.width, ctx.height, ColorType::Rgb)
            .context("Failed to encode image")?;

        Ok(output)
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(JpegEncoderBench)
}
