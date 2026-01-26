use anyhow::Result;
use benchmark_harness::{Args, BenchmarkImplementation};
use std::fs;

struct NullBench;

struct BenchContext {
    input_data: Vec<u8>,
    output_buffer: Vec<u8>,
}

impl BenchmarkImplementation for NullBench {
    fn name(&self) -> &'static str {
        "null-decode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let input_data = fs::read(&args.input)?;
        let output_buffer = vec![0u8; input_data.len()];

        Ok(Box::new(BenchContext {
            input_data,
            output_buffer,
        }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_mut::<BenchContext>()
            .expect("Invalid context");

        let checksum = crc32fast::hash(&ctx.input_data);
        ctx.output_buffer.copy_from_slice(&ctx.input_data);

        // Write checksum at end to prevent optimization
        if ctx.output_buffer.len() >= 4 {
            let len = ctx.output_buffer.len();
            ctx.output_buffer[len - 4..].copy_from_slice(&checksum.to_le_bytes());
        }

        Ok(ctx.output_buffer.clone())
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(NullBench)
}
