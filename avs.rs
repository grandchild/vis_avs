#![allow(non_camel_case_types, unused_attributes)]
/// Rust bindings for AVS
///
/// compile:
///     rustc --target i686-unknown-linux-gnu -L build_linux/ avs.rs -o avs_rs
/// run:
///     LD_LIBRARY_PATH=build_linux/ ./avs_rs ../presets/starfield.avs
///
use argh::FromArgs;
use linuxfb::{/*double::Buffer,*/ Framebuffer};
use minifb::{Key, Window, WindowOptions};

use std::ffi::{CStr, CString};
use std::io::{Error, Result};
use std::os::raw::{c_char, c_void};

type AVS_Handle = u32;

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub enum AVS_Pixel_Format {
    AVS_PIXEL_RGB0_8 = 0,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub enum AVS_Audio_Source {
    AVS_AUDIO_INTERNAL = 0,
    AVS_AUDIO_EXTERNAL = 1,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub enum AVS_Beat_Source {
    AVS_BEAT_INTERNAL = 0,
    AVS_BEAT_EXTERNAL = 1,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub enum AVS_Preset_Format {
    AVS_PRESET_FORMAT_JSON = 0,
    AVS_PRESET_FORMAT_LEGACY = 1,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct AVS_Version {
    pub major: u32,
    pub minor: u32,
    pub patch: u32,
    pub rc: u32,
    pub commit: [u8; 20],
    pub changes: [u8; 20],
}

#[link(name = "avs")]
extern "C-unwind" {
    fn avs_init(
        base_path: *const c_char,
        audio_source: AVS_Audio_Source,
        beat_source: AVS_Beat_Source,
    ) -> AVS_Handle;
    fn avs_render_frame(
        avs: AVS_Handle,
        framebuffer: *mut c_void,
        width: usize,
        height: usize,
        time_in_ms: i64,
        is_beat: bool,
        pixel_format: AVS_Pixel_Format,
    ) -> bool;
    fn avs_audio_set(
        avs: AVS_Handle,
        audio_left: *const i16,
        audio_right: *const i16,
        audio_length: usize,
        samples_per_second: usize,
    ) -> i32;
    fn avs_audio_device_count(avs: AVS_Handle) -> i32;
    fn avs_audio_device_names(avs: AVS_Handle) -> *const *const c_char;
    fn avs_audio_device_set(avs: AVS_Handle, device: i32) -> bool;
    fn avs_preset_load(avs: AVS_Handle, file_path: *const c_char) -> bool;
    fn avs_preset_set(avs: AVS_Handle, preset: *const c_char) -> bool;
    fn avs_preset_save(avs: AVS_Handle, file_path: *const c_char, indent: bool)
        -> bool;
    fn avs_preset_get(avs: AVS_Handle) -> *const c_char;
    fn avs_preset_set_legacy(
        avs: AVS_Handle,
        preset: *const u8,
        preset_length: usize,
    ) -> bool;
    fn avs_preset_get_legacy(
        avs: AVS_Handle,
        preset_length_out: *mut usize,
    ) -> *const u8;
    fn avs_preset_save_legacy(avs: AVS_Handle, file_path: *const c_char) -> bool;
    fn avs_error_str(avs: AVS_Handle) -> *const c_char;
    fn avs_free(avs: AVS_Handle);
    fn avs_version() -> AVS_Version;
}

#[derive(Debug)]
pub struct AVS {
    handle: AVS_Handle,
}

#[derive(Debug)]
pub struct AvsVersion {
    pub major: u32,
    pub minor: u32,
    pub patch: u32,
    pub rc: u32,
    pub commit: String,
    pub changes: String,
}

impl AVS {
    pub fn new(
        base_path: Option<&str>,
        audio_source: AVS_Audio_Source,
        beat_source: AVS_Beat_Source,
    ) -> Result<Self> {
        let base_path_str: CString;
        let base_path_ptr = match base_path {
            Some(path) => {
                base_path_str = CString::new(path).unwrap();
                base_path_str.as_ptr()
            }
            None => std::ptr::null(),
        };
        let handle = unsafe { avs_init(base_path_ptr, audio_source, beat_source) };
        if handle == 0 {
            let error_ptr = unsafe { avs_error_str(0) };
            if error_ptr.is_null() {
                return Err(Error::other("unknown error"));
            }
            let err_cstr = unsafe { CStr::from_ptr(error_ptr) }.to_string_lossy();
            Err(Error::other(err_cstr))
        } else {
            Ok(Self { handle })
        }
    }

    pub fn render_frame(
        &self,
        framebuffer: *mut c_void,
        width: usize,
        height: usize,
        time_in_ms: i64,
        is_beat: bool,
        pixel_format: AVS_Pixel_Format,
    ) -> Result<()> {
        if !unsafe {
            avs_render_frame(
                self.handle,
                framebuffer,
                width,
                height,
                time_in_ms,
                is_beat,
                pixel_format,
            )
        } {
            return Err(self.error("render_frame"));
        }
        Ok(())
    }

    pub fn audio_set(
        &self,
        audio_left: *const i16,
        audio_right: *const i16,
        audio_length: usize,
        samples_per_second: usize,
    ) -> i32 {
        unsafe {
            avs_audio_set(
                self.handle,
                audio_left,
                audio_right,
                audio_length,
                samples_per_second,
            )
        }
    }

    pub fn audio_devices(&self) -> Result<Vec<String>> {
        let count: isize = unsafe { avs_audio_device_count(self.handle) as isize };
        if count < 0 {
            return Err(self.error("audio_devices"));
        }
        let mut names = Vec::with_capacity(count as usize);
        let names_ptr = unsafe { avs_audio_device_names(self.handle) };
        if names_ptr.is_null() {
            return Err(self.error("audio_devices"));
        }
        for i in 0isize..count {
            let name_ptr = unsafe { *names_ptr.offset(i) };
            if name_ptr.is_null() {
                break;
            }
            let name = unsafe { CStr::from_ptr(name_ptr) }.to_string_lossy();
            names.push(name.to_string());
        }
        Ok(names)
    }

    pub fn audio_device_set(&self, device: i32) -> Result<()> {
        if !unsafe { avs_audio_device_set(self.handle, device) } {
            return Err(self.error("audio_device_set"));
        }
        Ok(())
    }

    pub fn preset_load(&self, file_path: &str) -> Result<()> {
        let file_path = CString::new(file_path).unwrap();
        if !unsafe { avs_preset_load(self.handle, file_path.as_ptr()) } {
            return Err(self.error("preset_load"));
        }
        Ok(())
    }

    pub fn preset_set(&self, preset: &str) -> Result<()> {
        let preset = CString::new(preset).unwrap();
        if !unsafe { avs_preset_set(self.handle, preset.as_ptr()) } {
            return Err(self.error("preset_set"));
        }
        Ok(())
    }

    pub fn preset_save(&self, file_path: &str, indent: bool) -> Result<()> {
        let file_path = CString::new(file_path).unwrap();
        if !unsafe { avs_preset_save(self.handle, file_path.as_ptr(), indent) } {
            return Err(self.error("preset_save"));
        }
        Ok(())
    }

    pub fn preset_get(&self) -> Result<String> {
        let preset_ptr = unsafe { avs_preset_get(self.handle) };
        if preset_ptr.is_null() {
            return Err(self.error("preset_get"));
        }
        let preset = unsafe { CStr::from_ptr(preset_ptr) }
            .to_str()
            .map_err(Error::other)?;
        Ok(preset.to_string())
    }

    pub fn preset_set_legacy(&self, preset: &[u8]) -> Result<()> {
        if !unsafe { avs_preset_set_legacy(self.handle, preset.as_ptr(), preset.len()) }
        {
            return Err(self.error("preset_set_legacy"));
        }
        Ok(())
    }

    pub fn preset_get_legacy(&self) -> Result<Vec<u8>> {
        let mut length = 0;
        let preset_ptr = unsafe { avs_preset_get_legacy(self.handle, &mut length) };
        if preset_ptr.is_null() || length == 0 {
            return Err(self.error("preset_get_legacy"));
        }
        let preset = unsafe { std::slice::from_raw_parts(preset_ptr, length) };
        Ok(preset.to_vec())
    }

    pub fn preset_save_legacy(&self, file_path: &str) -> Result<()> {
        let file_path = CString::new(file_path).unwrap();
        if !unsafe { avs_preset_save_legacy(self.handle, file_path.as_ptr()) } {
            return Err(self.error("preset_save_legacy"));
        }
        Ok(())
    }

    fn error(&self, prefix: &str) -> Error {
        let error_ptr = unsafe { avs_error_str(self.handle) };
        if error_ptr.is_null() {
            return Error::other("unknown AVS error");
        }
        // Safety: `error_ptr` is not null anymore here. What else?
        let err_msg = unsafe { CStr::from_ptr(error_ptr) }
            .to_string_lossy()
            .to_string();
        if err_msg.is_empty() {
            Error::other(prefix.to_string() + ": No error")
        } else {
            Error::other(prefix.to_string() + ": " + &err_msg)
        }
    }

    pub fn version() -> AvsVersion {
        let v = unsafe { avs_version() };
        AvsVersion {
            major: v.major,
            minor: v.minor,
            patch: v.patch,
            rc: v.rc,
            commit: String::from_utf8_lossy(&v.commit[..19]).to_string(),
            changes: String::from_utf8_lossy(&v.changes[..19]).to_string(),
        }
    }
}

impl Drop for AVS {
    fn drop(&mut self) {
        unsafe { avs_free(self.handle) }
    }
}

#[derive(FromArgs, Debug)]
/// AVS
struct AvsArgs {
    #[argh(positional)]
    preset: String,
    /// convert legacy preset to JSON
    #[argh(switch)]
    convert: bool,
}

fn main() -> Result<()> {
    let args: AvsArgs = argh::from_env();
    let avs = AVS::new(
        None,
        AVS_Audio_Source::AVS_AUDIO_INTERNAL,
        AVS_Beat_Source::AVS_BEAT_INTERNAL,
    )?;
    println!("{:?}", AVS::version());
    println!("{:?}", args);
    let preset = args.preset;
    avs.preset_load(&preset)?;
    if args.convert {
        let preset_filename = preset.to_string() + "-preset";
        println!("{:?}", avs.preset_save(&preset_filename, true)?);
        return Ok(());
    }
    // render a frame
    let mut width: usize = 640;
    let mut height: usize = 380;
    let (mut last_width, mut last_height) = (width, height);
    let mut framebuffer: Vec<u32> = vec![0; width * height];
    let is_beat = false;
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
        // Limit to max ~120 fps
        window
            .limit_update_rate(Some(std::time::Duration::from_micros(1_000_000 / 120)));

        while window.is_open()
            && !(window.is_key_down(Key::Escape) || window.is_key_down(Key::Q))
        {
            let (window_width, window_height) = window.get_size();
            // width = window_width & !3usize; // width must be multiple of 4
            // height = window_height & !1usize; // height must be multiple of 2
            width = 1000;
            height = 562;
            if width != last_width || height != last_height {
                framebuffer = vec![0; width * height];
                last_width = width;
                last_height = height;
                println!("resize: {}x{}", width, height);
            }
            avs.render_frame(
                framebuffer.as_mut_ptr() as *mut c_void,
                width,
                height,
                time_in_ms,
                is_beat,
                AVS_Pixel_Format::AVS_PIXEL_RGB0_8,
            )?;

            window
                .update_with_buffer(&framebuffer, width, height)
                .unwrap();
        }
    } else if let Some(fb_dev) = (Framebuffer::list().map_err(Error::other)?)
        .into_iter()
        .next()
    {
        println!("{:?}", fb_dev);
        let mut fb = Framebuffer::new(fb_dev)
            .map_err(|e| Error::other(format!("accessing linux fb device: {e:?}")))?;
        let (fb_width, fb_height) = fb.get_size();
        println!("{fb_width}x{fb_height}");
        fb.set_virtual_size(fb_width, fb_height).unwrap();
        // let mut buffer = Buffer::new(fb)
        //     .map_err(|e| format!("creating double buffer: {:?}", e))?;
        loop {
            // let current_fb = buffer.as_mut_slice();
            avs.render_frame(
                fb.map()
                    .map_err(|e| Error::other(format!("{e:?}")))?
                    .as_mut_ptr() as *mut c_void,
                fb_width as usize,
                fb_height as usize,
                time_in_ms,
                is_beat,
                AVS_Pixel_Format::AVS_PIXEL_RGB0_8,
            )?;
            // buffer
            //     .flip()
            //     .map_err(|e| format!("flipping linux fb double buffer: {:?}", e))?;
        }
    } else {
        return Err(Error::other("No framebuffer devices"));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_1() -> Result<()> {
        let avs = AVS::new(
            None,
            AVS_Audio_Source::AVS_AUDIO_INTERNAL,
            AVS_Beat_Source::AVS_BEAT_INTERNAL,
        )?;
        avs.preset_load("../presets/starfield.avs")?;
        assert_eq!(AVS::version().major, 2);
        Ok(())
    }
}
