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

    #[arg(long)]
    pub verify: bool,
    #[arg(long, default_value_t = 60.0)]
    pub verify_threshold: f64,
}

pub trait BenchmarkImplementation {
    fn name(&self) -> &'static str;

    /// Called once before the loop to prepare any resources (e.g. loading the image/data)
    /// Returns a context object that is passed to each iteration.
    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>>;

    /// The core operation to benchmark (encode or decode).
    /// `context` is the object returned by `prepare`.
    /// Should return the output bytes.
    /// If `discard` is true, the implementation SHOULD still produce the output data in memory so it can be checksummed.
    fn run(&self, args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>>;

    /// Called to verify the output against a reference.
    /// This is only called if `verify` is true.
    fn verify(&self, args: &Args, context: &dyn std::any::Any, output: &[u8]) -> Result<()>;
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

        if args.verify {
            impl_
                .verify(&args, context.as_ref(), &output)
                .context("Verification failed")?;
        }
    }

    Ok(())
}

pub fn calculate_psnr(a: &[u8], b: &[u8]) -> Result<f64> {
    if a.len() != b.len() {
        anyhow::bail!("Image dimensions/size mismatch: {} vs {}", a.len(), b.len());
    }

    let mut mse = 0.0;
    for i in 0..a.len() {
        let diff = a[i] as f64 - b[i] as f64;
        mse += diff * diff;
    }
    mse /= a.len() as f64;

    if mse == 0.0 {
        return Ok(f64::INFINITY);
    }

    Ok(10.0 * (255.0 * 255.0 / mse).log10())
}

/// Verify output for lossless formats (exact byte match)
pub fn verify_lossless(output: &[u8], reference: &[u8]) -> Result<()> {
    if output.len() != reference.len() {
        anyhow::bail!(
            "Lossless verification failed: size mismatch ({} vs {} bytes)",
            output.len(),
            reference.len()
        );
    }

    if output != reference {
        // Find first difference for debugging
        for (i, (a, b)) in output.iter().zip(reference.iter()).enumerate() {
            if a != b {
                anyhow::bail!(
                    "Lossless verification failed: byte mismatch at offset {i} (0x{a:02x} vs 0x{b:02x})"
                );
            }
        }
    }

    Ok(())
}

/// Verify output for lossy formats (PSNR-based)
pub fn verify_lossy(output: &[u8], reference: &[u8], threshold_db: f64) -> Result<()> {
    let psnr = calculate_psnr(output, reference)?;

    if psnr < threshold_db {
        anyhow::bail!(
            "Lossy verification failed: PSNR {psnr:.2} dB below threshold {threshold_db:.2} dB"
        );
    }

    Ok(())
}
