use anyhow::Result;
use benchmark_harness::{Args, BenchmarkImplementation};

struct NullEncodeBench;

struct BenchContext {
    input_data: Vec<u8>,
}

impl BenchmarkImplementation for NullEncodeBench {
    fn name(&self) -> &'static str {
        "null-encode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        let (_width, _height, rgb_data) = benchmark_harness::decode_ppm_rgb8(&args.input)?;
        Ok(Box::new(BenchContext {
            input_data: rgb_data,
        }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");

        let checksum = crc32fast::hash(&ctx.input_data);
        let mut output = ctx.input_data.clone();

        // Write checksum at end to prevent optimization
        if output.len() >= 4 {
            let len = output.len();
            output[len - 4..].copy_from_slice(&checksum.to_le_bytes());
        }

        Ok(output)
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(NullEncodeBench)
}
