use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation};
use zune_png::PngEncoder;

struct ZunePngBench;

struct BenchContext {
    input_data: Vec<u8>,
    width: usize,
    height: usize,
}

impl BenchmarkImplementation for ZunePngBench {
    fn name(&self) -> &'static str {
        "zune-png-encode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let img = image::open(&args.input).context("Failed to open input image")?;
        let width = img.width() as usize;
        let height = img.height() as usize;
        let input_data = img.to_rgb8().into_raw();

        Ok(Box::new(BenchContext {
            input_data,
            width,
            height,
        }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");

        // zune-png encoder API
        // PngEncoder::new(data, width, height, colorspace, depth, options)
        // I need to check the exact signature.
        // Assuming: PngEncoder::new(&data, options)

        // Let's try to construct options.
        // zune-core might be needed for options?
        // But zune-png might re-export them or have its own.

        // I'll try to use PngEncoder::new and see what happens.
        // If I can't find the API, I'll search again.

        // Based on zune-jpegxl, it used zune_core::options::EncoderOptions.
        // zune-png might be similar.
        // But zune-png dependency in Cargo.toml doesn't include zune-core explicitly?
        // It might be a transitive dependency.
        // I should add zune-core if needed.

        let options = zune_core::options::EncoderOptions::new(
            ctx.width,
            ctx.height,
            zune_core::colorspace::ColorSpace::RGB,
            zune_core::bit_depth::BitDepth::Eight,
        );

        let mut encoder = PngEncoder::new(&ctx.input_data, options);

        // output buffer
        let mut output = Vec::new();
        // encode expects a writer or buffer?
        // If encode takes 1 arg, it might be the writer/buffer?
        // But PngEncoder::new took data.
        // Maybe encode takes &mut Vec<u8>?

        // Let's try passing &mut output.
        // But output needs to be pre-allocated?
        // Or maybe it appends?

        // zune-jpegxl encode took &mut [u8] and returned bytes written.
        // If zune-png is similar, I should pre-allocate.
        let estimated_size = ctx.input_data.len(); // PNG can be larger or smaller
        output.resize(estimated_size + 1024, 0); // Add some buffer

        let bytes_written = encoder
            .encode(&mut output)
            .map_err(|e| anyhow::anyhow!("Failed to encode PNG: {e:?}"))?;
        output.truncate(bytes_written);

        Ok(output)
    }

    fn verify(&self, _args: &Args, _context: &dyn std::any::Any, output: &[u8]) -> Result<()> {
        if output.is_empty() {
            anyhow::bail!("Encoder produced empty output");
        }
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
    benchmark_harness::main(ZunePngBench)
}
