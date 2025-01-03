#![allow(non_camel_case_types, unused_attributes)]

/// Rust bindings for AVS
///
/// compile:
///     rustc --target i686-unknown-linux-gnu -L build_linux/ avs.rs -o avs_rs
/// run:
///     LD_LIBRARY_PATH=build_linux/ ./avs_rs ../presets/starfield.avs
///
use anyhow::{anyhow, Result};
use argh::FromArgs;
use avs_rs::{AvsComponentPosition, AvsPixelFormat};
use inotify::{Inotify, WatchMask};
#[cfg(target_os = "linux")]
use linuxfb::{/*double::Buffer,*/ Framebuffer};
use minifb::{Key, KeyRepeat, Window, WindowOptions};

use std::rc::Rc;
use std::{io::ErrorKind, path::Path};

use avs_rs::{Avs, AvsAudioSource, AvsBeatSource, AvsBufferData, AvsError, AvsFramebuffer};

/// AVS
#[derive(FromArgs, Debug)]
struct AvsArgs {
    #[argh(positional)]
    preset: String,
    /// convert legacy preset to JSON
    #[argh(switch)]
    convert: bool,
}

fn main() -> Result<()> {
    let args: AvsArgs = argh::from_env();
    let mut avs = Avs::new(None, AvsAudioSource::Internal, AvsBeatSource::External)?;
    println!("{:?}", Avs::lib_version());
    // save preset_format_schema output to a file
    std::fs::write("avs-preset.v0-0.schema.json", Avs::preset_format_schema()?)?;
    println!("{:?}", args);
    match avs.audio_devices() {
        Ok(devices) => {
            for device in devices {
                println!("{:?}", device);
            }
        }
        Err(err) => eprintln!("{err}"),
    };
    let preset = Path::new(&args.preset);
    avs.preset_load(preset)?;
    for line in avs.print_component_tree() {
        println!("{line}");
    }
    let mut inotify = Inotify::init()?;
    inotify.watches().add(preset, WatchMask::MODIFY)?;
    let mut inotify_event_buffer = [0; 1024];
    // let el = avs.first_of("Effect List").ok_or(AvsError::with_str(
    //     "cannot find effect Effect List or no component of that type",
    // ))?;
    // avs.create("Grain", &el, AvsComponentPosition::Child);
    if args.convert {
        let preset_filename = args.preset.to_string() + "-preset";
        println!(
            "{:?}",
            avs.preset_save(Path::new(&preset_filename), false, true)?
        );
        return Ok(());
    }
    // render a frame
    let width: usize = 640;
    let height: usize = 380;
    let (mut last_width, mut last_height) = (width, height);
    let mut framebuffer: Vec<u32> = vec![0; width * height];
    let mut is_beat = false;
    let time_in_ms = -1; // realtime mode

    if let Ok(mut window) = Window::new(
        "AVS",
        width,
        height,
        WindowOptions {
            resize: true,
            transparency: false,
            borderless: true,
            ..WindowOptions::default()
        },
    ) {
        window.set_target_fps(120);

        let mut avs_buffer_data = AvsBufferData::Vec(&mut framebuffer);
        let mut avs_framebuffer = AvsFramebuffer {
            data: &mut avs_buffer_data,
            width,
            height,
        };
        while window.is_open() && !(window.is_key_down(Key::Escape) || window.is_key_down(Key::Q)) {
            // let (window_width, window_height) = window.get_size();
            // width = window_width & !3usize; // width must be multiple of 4
            // height = window_height & !1usize; // height must be multiple of 2
            if width != last_width || height != last_height {
                avs_framebuffer.resize(width, height)?;
                last_width = width;
                last_height = height;
                println!("resize: {}x{}", width, height);
            }
            is_beat = window.is_key_pressed(Key::Space, KeyRepeat::No);
            avs.render_frame(
                &mut avs_framebuffer,
                time_in_ms,
                is_beat,
                AvsPixelFormat::Rgb0_8,
            )?;

            match *avs_framebuffer.data {
                AvsBufferData::Vec(ref v) => window.update_with_buffer(*v, width, height).unwrap(),
                _ => panic!(),
            }

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
                Err(err) if err.kind() == ErrorKind::WouldBlock => continue,
                Err(err) => eprintln!("!! inotify {err}"),
            }
        }
    } else {
        try_run_in_linuxfb(avs, time_in_ms, is_beat)?;
    }
    Ok(())
}

#[cfg(target_os = "linux")]
fn try_run_in_linuxfb(avs: Avs, time_in_ms: i64, is_beat: bool) -> Result<()> {
    if let Some(fb_dev) = (Framebuffer::list()?).into_iter().next() {
        println!("{:?}", fb_dev);
        let mut fb =
            Framebuffer::new(fb_dev).map_err(|e| anyhow!("accessing linux fb device: {e:?}"))?;
        let (fb_width, fb_height) = fb.get_size();
        println!("{fb_width}x{fb_height}");
        fb.set_virtual_size(fb_width, fb_height).unwrap();
        // let mut buffer = Buffer::new(fb)
        //     .map_err(|e| format!("creating double buffer: {:?}", e))?;
        let mut avs_buffer_data = AvsBufferData::Mmap(fb.map().map_err(|e| anyhow!("{e:?}"))?);
        let mut avs_framebuffer = AvsFramebuffer {
            data: &mut avs_buffer_data,
            width: fb_width as usize,
            height: fb_height as usize,
        };
        loop {
            // let current_fb = buffer.as_mut_slice();
            avs.render_frame(
                &mut avs_framebuffer,
                time_in_ms,
                is_beat,
                AvsPixelFormat::Rgb0_8,
            )?;
            // buffer.flip()?;
        }
    } else {
        Ok(())
        // Err(std::io::Error::other("No framebuffer devices"))
    }
}

#[cfg(not(target_os = "linux"))]
fn try_run_in_linuxfb(_avs: Avs, _time_in_ms: i64, _is_beat: bool) -> Result<()> {
    Err(anyhow!("Not implemented for this platform"))
}
