use anyhow::Result;
use benchmark_harness::{Args, BenchmarkImplementation};
use std::fs;

struct NullBench;

struct BenchContext {
    input_data: Vec<u8>,
}

impl BenchmarkImplementation for NullBench {
    fn name(&self) -> &'static str {
        "null-decode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let input_data = fs::read(&args.input)?;
        Ok(Box::new(BenchContext { input_data }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");

        let checksum = crc32fast::hash(&ctx.input_data);
        let mut result = vec![0u8; ctx.input_data.len()];
        result.copy_from_slice(&ctx.input_data);

        // Write checksum at end to prevent optimization
        if result.len() >= 4 {
            let len = result.len();
            result[len - 4..].copy_from_slice(&checksum.to_le_bytes());
        }

        Ok(result)
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(NullBench)
}
