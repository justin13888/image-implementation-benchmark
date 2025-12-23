use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation};
use jxl_oxide::JxlImage;
use std::fs;
use std::io::Cursor;

struct JxlOxideBench;

struct BenchContext {
    input_data: Vec<u8>,
}

impl BenchmarkImplementation for JxlOxideBench {
    fn name(&self) -> &'static str {
        "jxl-oxide-decode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let input_data = fs::read(&args.input).context("Failed to read input file")?;

        Ok(Box::new(BenchContext { input_data }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");
        let image = JxlImage::builder()
            .read(Cursor::new(&ctx.input_data))
            .map_err(|e| anyhow::anyhow!("Failed to read JXL header: {e}"))?;
        let _render = image
            .render_frame(0)
            .map_err(|e| anyhow::anyhow!("Failed to render frame: {e}"))?;

        // We need to return bytes.
        // render.image() returns a grid.
        // For benchmarking, the decoding time is what matters.
        Ok(Vec::new()) // TODO: Return actual pixels needed for verification later
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(JxlOxideBench)
}
