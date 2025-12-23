use anyhow::{Context, Result};
use benchmark_harness::{Args, BenchmarkImplementation, Quality};
use rav1e::prelude::*;

struct Rav1eBench;

struct BenchContext {
    width: usize,
    height: usize,
    rgb_data: Vec<u8>,
    quality: Quality,
}

impl BenchmarkImplementation for Rav1eBench {
    fn name(&self) -> &'static str {
        "rav1e-encode"
    }

    fn prepare(&self, args: &Args) -> Result<Box<dyn std::any::Any>> {
        // Load the raw image data (PPM format expected)
        let img = image::open(&args.input).context("Failed to open input image")?;
        let width = img.width() as usize;
        let height = img.height() as usize;
        let rgb_data = img.to_rgb8().into_raw();
        let quality = args.quality;

        Ok(Box::new(BenchContext {
            width,
            height,
            rgb_data,
            quality,
        }))
    }

    fn run(&self, _args: &Args, context: &mut dyn std::any::Any) -> Result<Vec<u8>> {
        let ctx = context
            .downcast_ref::<BenchContext>()
            .expect("Invalid context");

        // Configure encoder
        let mut enc_config = EncoderConfig {
            width: ctx.width,
            height: ctx.height,
            time_base: Rational::new(1, 30),
            bit_depth: 8,
            chroma_sampling: ChromaSampling::Cs420,
            ..Default::default()
        };

        match ctx.quality {
            Quality::WebLow => {
                enc_config.quantizer = 100;
                enc_config.speed_settings = SpeedSettings::from_preset(9);
            }
            Quality::WebHigh => {
                enc_config.quantizer = 80;
                enc_config.speed_settings = SpeedSettings::from_preset(7);
            }
            Quality::Archival => {
                enc_config.quantizer = 50;
                enc_config.speed_settings = SpeedSettings::from_preset(4);
            }
        }

        let cfg = Config::new().with_encoder_config(enc_config);

        let mut enc_ctx = cfg
            .new_context::<u8>()
            .context("Failed to create encoder context")?;

        // Create frame and convert RGB to YUV
        let mut frame = enc_ctx.new_frame();

        // Cache strides to avoid borrow conflicts
        let y_stride = frame.planes[0].cfg.stride;

        // Simple RGB to YUV420 conversion
        for y in 0..ctx.height {
            for x in 0..ctx.width {
                let idx = (y * ctx.width + x) * 3;
                let r = ctx.rgb_data[idx] as f32;
                let g = ctx.rgb_data[idx + 1] as f32;
                let b = ctx.rgb_data[idx + 2] as f32;
                // BT.709 conversion
                let y_val = (0.2126 * r + 0.7152 * g + 0.0722 * b) as u8;
                frame.planes[0].data_origin_mut()[y * y_stride + x] = y_val;
            }
        }

        // Subsample for U and V planes (4:2:0)
        let u_stride = frame.planes[1].cfg.stride;
        let v_stride = frame.planes[2].cfg.stride;
        for y in (0..ctx.height).step_by(2) {
            for x in (0..ctx.width).step_by(2) {
                let idx = (y * ctx.width + x) * 3;
                let r = ctx.rgb_data[idx] as f32;
                let g = ctx.rgb_data[idx + 1] as f32;
                let b = ctx.rgb_data[idx + 2] as f32;

                let y_val = 0.2126 * r + 0.7152 * g + 0.0722 * b;
                let u_val = ((b - y_val) * 0.539 + 128.0).clamp(0.0, 255.0) as u8;
                let v_val = ((r - y_val) * 0.635 + 128.0).clamp(0.0, 255.0) as u8;

                let uv_y = y / 2;
                let uv_x = x / 2;
                frame.planes[1].data_origin_mut()[uv_y * u_stride + uv_x] = u_val;
                frame.planes[2].data_origin_mut()[uv_y * v_stride + uv_x] = v_val;
            }
        }

        enc_ctx.send_frame(frame).context("Failed to send frame")?;
        enc_ctx.flush();

        let mut output = Vec::new();
        loop {
            match enc_ctx.receive_packet() {
                Ok(pkt) => {
                    output.extend_from_slice(&pkt.data);
                }
                Err(EncoderStatus::Encoded) | Err(EncoderStatus::LimitReached) => break,
                Err(e) => anyhow::bail!("Encoding error: {e:?}"),
            }
        }

        Ok(output)
    }
}

fn main() -> Result<()> {
    benchmark_harness::main(Rav1eBench)
}
