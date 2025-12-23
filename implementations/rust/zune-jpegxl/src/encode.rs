use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation, Quality};
use zune_core::bit_depth::BitDepth;
use zune_core::colorspace::ColorSpace;
use zune_core::options::EncoderOptions;
use zune_jpegxl::JxlSimpleEncoder;

struct ZuneJxlBench;

struct BenchContext {
    input_data: Vec<u8>,
    width: usize,
    height: usize,
    quality: Quality,
}

impl BenchmarkImplementation for ZuneJxlBench {
    fn name(&self) -> &'static str {
        "zune-jpegxl-encode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        // Load raw image
        let img = image::open(&args.input).context("Failed to open input image")?;
        let width = img.width() as usize;
        let height = img.height() as usize;
        let input_data = img.to_rgb8().into_raw();

        Ok(Box::new(BenchContext {
            input_data,
            width,
            height,
            quality: args.quality,
        }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");

        // TODO: see if zune exposes distance parameter as something not quality setting.
        let (quality, effort) = match ctx.quality {
            Quality::WebLow => (50, 7),    // Approximate d4.0
            Quality::WebHigh => (90, 7),   // Approximate d1.0
            Quality::Archival => (100, 9), // lossless
        };
        let options = EncoderOptions::new(ctx.width, ctx.height, ColorSpace::RGB, BitDepth::Eight)
            .set_effort(effort)
            .set_quality(quality);
        let encoder = JxlSimpleEncoder::new(&ctx.input_data, options);

        // Create output buffer - estimate size needed
        let estimated_size = ctx.input_data.len() * 2;
        let mut output = vec![0u8; estimated_size];

        let bytes_written = encoder
            .encode(&mut output[..])
            .context("Failed to encode JXL")?;

        output.truncate(bytes_written);
        Ok(output)
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(ZuneJxlBench)
}
