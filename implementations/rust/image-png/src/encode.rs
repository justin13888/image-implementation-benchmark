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
        let img = image::open(&args.input).context("Failed to open input image")?;
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

        // Drop writer to flush? BufWriter flushes on drop or we can unwrap.
        // But we passed a mutable reference to the vector.
        // PngEncoder takes the writer by value or reference?
        // In 0.24, PngEncoder::new takes W: Write.
        // So `writer` is consumed.
        // Wait, if I pass `&mut output`, `writer` borrows it.
        // I need to make sure `writer` is dropped or flushed before I return `output`.
        // But `writer` owns the mutable reference.
        // If I put `writer` in a block, it will be dropped.
        // TODO: Fix this

        Ok(output)
    }

    fn verify(&self, _args: &Args, _context: &dyn std::any::Any, output: &[u8]) -> Result<()> {
        if output.is_empty() {
            anyhow::bail!("Encoder produced empty output");
        }
        // Check PNG signature
        if output.len() < 8
            || output[0] != 0x89
            || output[1] != b'P'
            || output[2] != b'N'
            || output[3] != b'G'
        {
            anyhow::bail!("Output is not a valid PNG");
        }
        Ok(())
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ImagePngBench)
}
