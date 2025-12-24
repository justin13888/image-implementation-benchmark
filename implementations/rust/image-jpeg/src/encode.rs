use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation, Quality};
use image::codecs::jpeg::JpegEncoder;
use image::ImageEncoder;

struct ImageJpegBench;

struct BenchContext {
    img: image::DynamicImage,
    quality: u8,
}

impl BenchmarkImplementation for ImageJpegBench {
    fn name(&self) -> &'static str {
        "image-jpeg-encode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        // Load input image (PPM)
        let input_data = std::fs::read(&args.input).context("Failed to read input file")?;
        let img = image::load_from_memory_with_format(&input_data, image::ImageFormat::Pnm)
            .context("Failed to decode input PPM")?;

        let img = image::DynamicImage::ImageRgb8(img.to_rgb8());

        // Map quality to JPEG quality (1-100)
        let quality = match args.quality {
            Quality::WebLow => 50,
            Quality::WebHigh => 80,
            Quality::Archival => 95,
        };

        Ok(Box::new(BenchContext { img, quality }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");

        let mut output = Vec::with_capacity(ctx.img.as_bytes().len() / 2);
        {
            let encoder = JpegEncoder::new_with_quality(&mut output, ctx.quality);
            encoder
                .write_image(
                    ctx.img.as_bytes(),
                    ctx.img.width(),
                    ctx.img.height(),
                    ctx.img.color().into(),
                )
                .context("Failed to encode JPEG")?;
        }

        Ok(output)
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ImageJpegBench)
}
