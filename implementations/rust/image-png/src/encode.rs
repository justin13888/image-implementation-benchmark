use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation, Quality};
use image::codecs::png::{CompressionType, FilterType, PngEncoder};
use image::ImageEncoder;
use std::io::BufWriter;

struct ImagePngBench;

struct BenchContext {
    img: image::DynamicImage,
    compression: CompressionType,
    filter: FilterType,
}

impl BenchmarkImplementation for ImagePngBench {
    fn name(&self) -> &'static str {
        "image-png-encode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        // Load input image (PPM)
        let input_data = std::fs::read(&args.input).context("Failed to read input file")?;
        let img = image::load_from_memory_with_format(&input_data, image::ImageFormat::Pnm)
            .context("Failed to decode input PPM")?;
        let img = image::DynamicImage::ImageRgb8(img.to_rgb8());

        // Map quality to compression
        let (compression, filter) = match args.quality {
            Quality::WebLow => (CompressionType::Fast, FilterType::Sub),
            Quality::WebHigh => (CompressionType::Default, FilterType::Paeth),
            Quality::Archival => (CompressionType::Best, FilterType::Adaptive),
        };

        Ok(Box::new(BenchContext {
            img,
            compression,
            filter,
        }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");

        let mut output = Vec::with_capacity(ctx.img.as_bytes().len() / 2);
        {
            let writer = BufWriter::new(&mut output);
            let encoder = PngEncoder::new_with_quality(writer, ctx.compression, ctx.filter);
            encoder
                .write_image(
                    ctx.img.as_bytes(),
                    ctx.img.width(),
                    ctx.img.height(),
                    ctx.img.color().into(),
                )
                .context("Failed to encode PNG")?;
        }

        Ok(output)
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ImagePngBench)
}
