use anyhow::{Context, Result};
use clap::{Parser, ValueEnum};
use std::fs;
use std::path::PathBuf;

#[cfg(feature = "mimalloc")]
use mimalloc::MiMalloc;

#[cfg(feature = "mimalloc")]
#[global_allocator]
static GLOBAL: MiMalloc = MiMalloc;

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum, Debug)]
pub enum Quality {
    WebLow,
    WebHigh,
    Archival,
}

#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
pub struct Args {
    #[arg(long)]
    pub input: PathBuf,

    #[arg(long)]
    pub output: PathBuf,

    #[arg(long, value_enum)]
    pub quality: Quality,

    #[arg(long, default_value_t = 10)]
    pub iterations: u32,

    #[arg(long, default_value_t = 2)]
    pub warmup: u32,

    #[arg(long, default_value_t = 0)]
    pub threads: usize,

    #[arg(long)]
    pub discard: bool,
}

pub trait BenchmarkImplementation {
    fn name(&self) -> &'static str;

    /// Called once before the loop to prepare any resources (e.g. loading the image/data)
    /// Returns a context object that is passed to each iteration.
    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>>;

    /// The core operation to benchmark (encode or decode).
    /// `context` is the object returned by `prepare`.
    /// Should return the output bytes.
    fn run(&self, args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>>;
}

pub fn main<I: BenchmarkImplementation>(impl_: I) -> Result<()> {
    let args = Args::parse();

    // Set thread count
    if args.threads > 0 {
        std::env::set_var("RAYON_NUM_THREADS", args.threads.to_string());
        // Also try to set OMP_NUM_THREADS for C libraries
        std::env::set_var("OMP_NUM_THREADS", args.threads.to_string());
    }

    let mut context = impl_
        .prepare(&args)
        .context("Failed to prepare benchmark")?;

    // Warmup
    for _ in 0..args.warmup {
        let _ = impl_
            .run(&args, context.as_mut())
            .context("Warmup iteration failed")?;
    }

    // Measurement loop
    for _ in 0..args.iterations {
        let output = impl_
            .run(&args, context.as_mut())
            .context("Benchmark iteration failed")?;

        if args.discard {
            let mut hasher = crc32fast::Hasher::new();
            hasher.update(&output);
            let checksum = hasher.finalize();
            // Prevent optimization
            std::hint::black_box(checksum);
        } else if !output.is_empty() {
            fs::write(&args.output, &output).context("Failed to write output")?;
        }
    }

    Ok(())
}
