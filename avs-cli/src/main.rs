use std::path::Path;

use avs_rs::{
    Avs, AvsAudioSource, AvsBeatSource, AvsBufferData, AvsFramebuffer, AvsParameter,
    AvsParameterType, AvsPixelFormat,
};

use anyhow::{anyhow, bail, Result};
use argh::FromArgs;
use inotify::{Inotify, WatchMask};
#[cfg(target_os = "linux")]
use linuxfb::{/*double::Buffer,*/ Framebuffer};
use minifb::{Key, KeyRepeat, MouseButton, Window, WindowOptions};

mod keyboardmap;
use keyboardmap::AvsKeyboardMap;

/// AVS
#[derive(FromArgs, Debug)]
struct AvsArgs {
    #[argh(positional)]
    preset: Option<String>,
    /// only render a single frame
    #[argh(switch)]
    single_frame: bool,
    #[argh(subcommand)]
    mode: Mode,
}

#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand)]
enum Mode {
    Minifb(MinifbArgs),
    Linuxfb(LinuxfbArgs),
    Image(ImageArgs),
    Video(VideoArgs),
    Debug(DebugArgs),
}

/// Run minimal windowed mode
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "minifb")]
struct MinifbArgs {}

/// Run in linux framebuffer
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "linuxfb")]
struct LinuxfbArgs {}

/// Render one image
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "image")]
struct ImageArgs {
    /// output path
    #[argh(option, default = "\"output.png\".to_string()")]
    output: String,
}

/// Render video
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "video")]
struct VideoArgs {
    /// input audio file, if not provided, live input device audio will be used
    #[argh(option)]
    input: Option<String>,
    /// length in seconds, if not provided, length of audio is used
    #[argh(option)]
    length: Option<f32>,
    /// framerate in frames per second
    #[argh(option, default = "30f32")]
    framerate: f32,
    /// output path
    #[argh(option, default = "\"output.mp4\".to_string()")]
    output: String,
}

/// Debugging tasks
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "debug")]
struct DebugArgs {
    /// available tasks:
    /// "effects": print all effects and their config fields;
    /// "convert": convert legacy preset to new JSON format;
    /// "spec": save current avs preset JSON spec
    #[argh(option)]
    task: String,
}

fn main() -> Result<()> {
    let args: AvsArgs = argh::from_env();
    let mut avs =
        Avs::new(None, AvsAudioSource::Internal, AvsBeatSource::External, None)?;
    let preset = match &args.preset {
        Some(p) => {
            let preset = Path::new(p);
            avs.preset_load(preset)?;
            preset
        }
        None => Path::new(""),
    };

    let width: usize = 640;
    let height: usize = 380;
    let time_in_ms = -1; // realtime mode
    match args.mode {
        Mode::Minifb(_) => run_minifb_backend(
            avs,
            width,
            height,
            time_in_ms,
            preset,
            args.single_frame,
        ),
        Mode::Linuxfb(_) => {
            if cfg!(target_os = "linux") {
                run_linuxfb_backend(avs, time_in_ms, args.single_frame)
            } else {
                Err(anyhow!("Backend 'linuxfb' is only supported on Linux"))
            }
        }
        Mode::Image(image_args) => render_image(avs, width, height, &image_args.output),
        Mode::Video(video_args) => render_video(
            avs,
            width,
            height,
            video_args.input,
            video_args.length,
            video_args.framerate,
            &video_args.output,
        ),
        Mode::Debug(debug_args) => match debug_args.task.as_str() {
            "effects" => print_effect_lib(avs),
            "convert" => convert_preset(avs, args.preset),
            "spec" => Ok(std::fs::write(
                "avs-preset.v0-0.schema.json",
                Avs::preset_format_schema()?,
            )?),
            _ => Err(anyhow!("Unknown debug task: {}", debug_args.task)),
        },
    }
}

fn run_minifb_backend(
    mut avs: Avs,
    width: usize,
    height: usize,
    time_in_ms: i64,
    preset: &Path,
    single_frame: bool,
) -> Result<()> {
    let mut inotify = Inotify::init()?;
    inotify.watches().add(preset, WatchMask::MODIFY)?;
    let mut inotify_event_buffer = [0; 1024];

    let (mut last_width, mut last_height) = (width, height);
    let mut framebuffer: Vec<u32> = vec![0; width * height];
    let mut is_beat;

    let mut window = Window::new(
        "AVS",
        width,
        height,
        WindowOptions { resize: true, transparency: false, ..WindowOptions::default() },
    )?;
    window.set_target_fps(120);

    let mut avs_buffer_data = AvsBufferData::Vec(&mut framebuffer);
    let mut avs_framebuffer =
        AvsFramebuffer { data: &mut avs_buffer_data, width, height };
    let mut last_time = std::time::Instant::now();
    let keyboard_map = AvsKeyboardMap::new();
    while window.is_open()
        && !(window.is_key_down(Key::Escape) || window.is_key_down(Key::Q))
    {
        let (window_width, window_height) = window.get_size();
        if width != last_width || height != last_height {
            avs_framebuffer.resize(width, height)?;
            last_width = width;
            last_height = height;
            println!("resize: {width}x{height}");
        }
        is_beat = window.is_key_pressed(Key::Space, KeyRepeat::No);
        if let Some(mouse) = window.get_mouse_pos(minifb::MouseMode::Pass) {
            let _ = avs.input_mouse_pos_set(
                mouse.0 as f64 / (window_width as f64),
                mouse.1 as f64 / (window_height as f64),
            );
        };
        avs.input_mouse_button_set(0, window.get_mouse_down(MouseButton::Left))?;
        avs.input_mouse_button_set(1, window.get_mouse_down(MouseButton::Right))?;
        avs.input_mouse_button_set(2, window.get_mouse_down(MouseButton::Middle))?;
        let mut keys: [bool; 256] = [false; 256];
        for key in window.get_keys() {
            for k in keyboard_map.from_minifb(key) {
                keys[k as usize] = true;
            }
        }
        for (i, key) in keys.iter().enumerate() {
            avs.input_key_set(i as u32, *key)?;
        }
        avs.render_frame(
            &mut avs_framebuffer,
            time_in_ms,
            is_beat,
            AvsPixelFormat::Rgb0_8,
        )?;
        match *avs_framebuffer.data {
            AvsBufferData::Vec(ref v) => {
                window.update_with_buffer(v, width, height).unwrap()
            }
            _ => panic!(),
        }
        if single_frame {
            break;
        }

        let now = std::time::Instant::now();
        let _fps = 1_000_000_000 / (now - last_time).as_nanos();
        last_time = now;

        match inotify.read_events(&mut inotify_event_buffer) {
            Ok(events) => {
                for event in events {
                    if event.mask.contains(inotify::EventMask::MODIFY) {
                        println!("{} modified, reloading", preset.to_string_lossy());
                        match avs.preset_load(preset) {
                            Ok(_) => {
                                for line in avs.print_component_tree() {
                                    println!("{line}");
                                }
                            }
                            Err(err) => eprintln!("{err}"),
                        }
                    }
                }
            }
            Err(err) if err.kind() == std::io::ErrorKind::WouldBlock => continue,
            Err(err) => eprintln!("!! inotify {err}"),
        }
    }

    Ok(())
}

#[cfg(target_os = "linux")]
fn run_linuxfb_backend(avs: Avs, time_in_ms: i64, single_frame: bool) -> Result<()> {
    if let Some(fb_dev) = (Framebuffer::list()?).into_iter().next() {
        println!("{fb_dev:?}");
        let mut fb = Framebuffer::new(fb_dev)
            .map_err(|e| anyhow!("accessing linux fb device: {e:?}"))?;
        let (fb_width, fb_height) = fb.get_size();
        println!("{fb_width}x{fb_height}");
        fb.set_virtual_size(fb_width, fb_height).unwrap();
        // let mut buffer = Buffer::new(fb)
        //     .map_err(|e| format!("creating double buffer: {:?}", e))?;
        let mut avs_buffer_data =
            AvsBufferData::MmapLegacy(fb.map().map_err(|e| anyhow!("{e:?}"))?);
        let mut avs_framebuffer = AvsFramebuffer {
            data: &mut avs_buffer_data,
            width: fb_width as usize,
            height: fb_height as usize,
        };
        if single_frame {
            avs.render_frame(
                &mut avs_framebuffer,
                time_in_ms,
                false,
                AvsPixelFormat::Rgb0_8,
            )?;
        } else {
            loop {
                // let current_fb = buffer.as_mut_slice();
                avs.render_frame(
                    &mut avs_framebuffer,
                    time_in_ms,
                    false,
                    AvsPixelFormat::Rgb0_8,
                )?;
            }
        }
    }
    Ok(())
}

#[cfg(not(target_os = "linux"))]
fn run_linuxfb_backend(_avs: Avs, _time_in_ms: i64, _check_mode: bool) -> Result<()> {
    Err(anyhow!("Framebuffer device not available on this platform"))
}

fn render_image(
    avs: Avs,
    width: usize,
    height: usize,
    output_path: &str,
) -> Result<()> {
    let mut framebuffer: Vec<u32> = vec![0; width * height];
    let mut avs_buffer_data = AvsBufferData::Vec(&mut framebuffer);
    let mut avs_framebuffer =
        AvsFramebuffer { data: &mut avs_buffer_data, width, height };
    let mut warmup_frames: i64 = avs.root().config["Warmup Frames"].get_value()?;
    if warmup_frames < 0 {
        warmup_frames = 100;
    }
    for _ in 0..warmup_frames {
        avs.render_frame(&mut avs_framebuffer, -1, false, AvsPixelFormat::Rgb0_8)?;
    }
    let mut encoder = png::Encoder::new(
        std::fs::File::create(output_path)?,
        width as u32,
        height as u32,
    );
    encoder.set_color(png::ColorType::Rgba);
    encoder.set_depth(png::BitDepth::Eight);
    let mut writer = encoder.write_header()?;
    match *avs_framebuffer.data {
        AvsBufferData::Vec(ref v) => {
            let mut png_data: Vec<u8> = Vec::with_capacity(width * height * 4);
            for &pixel in v.iter() {
                let r = ((pixel >> 16) & 0xFF) as u8;
                let g = ((pixel >> 8) & 0xFF) as u8;
                let b = (pixel & 0xFF) as u8;
                png_data.push(r);
                png_data.push(g);
                png_data.push(b);
                png_data.push(255); // alpha
            }
            writer.write_image_data(&png_data)?;
        }
        _ => bail!("Unsupported buffer data for image output"),
    }
    Ok(())
}

fn render_video(
    _avs: Avs,
    _width: usize,
    _height: usize,
    _input_audio_path: Option<String>,
    _length_in_seconds: Option<f32>,
    _framerate: f32,
    _output_video_path: &str,
) -> Result<()> {
    todo!();
}

fn print_effect_lib(avs: Avs) -> Result<()> {
    for effect in avs.effects() {
        println!("{} [{}]:", effect.name, effect.handle);
        print_effect_params(&effect.parameters, 1);
    }
    Ok(())
}
fn print_effect_params(parameters: &Vec<AvsParameter>, indent: usize) {
    for param in parameters {
        for _ in 0..indent {
            print!("  ");
        }
        println!("- !{:?} {} [{}]", param.r#type, param.name, param.handle);
        if matches!(param.r#type, AvsParameterType::List) {
            print_effect_params(&param.children, indent + 1);
        }
    }
}

fn convert_preset(avs: Avs, filename: Option<String>) -> Result<()> {
    match filename {
        Some(p) => {
            if p.ends_with(".avs-preset") {
                println!("convert: Nothing to do for {p}, already in new format.");
                return Ok(());
            }
            if !p.ends_with(".avs") {
                bail!("convert: Extension is not .avs, cannot convert preset: {p}");
            }
            let new_name = p + "-preset";
            println!("{:?}", avs.preset_save(Path::new(&new_name), false, true)?);
            Ok(())
        }
        None => bail!("Preset filename must be provided for 'convert' task"),
    }
}
