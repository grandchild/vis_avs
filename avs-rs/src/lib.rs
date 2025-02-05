#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!("./bindings.rs");

// use std::cell::RefCell;
use std::collections::BTreeMap;
use std::error::Error;
use std::ffi::{CStr, CString};
use std::fmt;
use std::os::raw::c_void;
use std::path::Path;
use std::rc::Rc;
use std::slice;

use memmap::MmapMut;

mod c_enum;
use c_enum::FromCEnum;

mod tree;
use tree::InsertOnlyTree;

use avs_binding_macros::FromCEnum;

#[derive(Debug, Default, FromCEnum, PartialEq)]
#[repr(u32)]
pub enum AvsPixelFormat {
    #[default]
    Rgb0_8 = AVS_PIXEL_RGB0_8,
}

#[derive(Debug, Default, FromCEnum, PartialEq)]
#[repr(u32)]
pub enum AvsAudioSource {
    #[default]
    Internal = AVS_AUDIO_INTERNAL,
    External = AVS_AUDIO_EXTERNAL,
}

#[derive(Debug, Default, FromCEnum, PartialEq)]
#[repr(u32)]
pub enum AvsBeatSource {
    #[default]
    Internal = AVS_BEAT_INTERNAL,
    External = AVS_BEAT_EXTERNAL,
}

#[derive(Default)]
pub struct Avs {
    handle: AVS_Handle,
    pub pixel_format: AvsPixelFormat,
    pub effect_library: AvsEffectLibrary,
    preset: InsertOnlyTree<AvsComponent>,
}

impl Avs {
    pub fn new(
        base_path: Option<&str>,
        audio_source: AvsAudioSource,
        beat_source: AvsBeatSource,
    ) -> Result<Self, AvsError> {
        let base_path_ptr = match base_path {
            Some(path) => {
                let base_path_str = CString::new(path).unwrap();
                base_path_str.as_ptr()
            }
            None => std::ptr::null(),
        };
        let handle = unsafe {
            avs_init(base_path_ptr, audio_source.to_value(), beat_source.to_value())
        };
        if handle == 0 {
            return Err(Avs::default().error("init"));
        }

        let effect_library = Avs::effect_library(handle)?;
        let mut avs = Avs {
            handle,
            pixel_format: AvsPixelFormat::Rgb0_8,
            effect_library,
            preset: InsertOnlyTree::new(),
        };
        avs.reload_component_tree()?;
        Ok(avs)
    }

    pub fn render_frame(
        &self,
        framebuffer: &mut AvsFramebuffer,
        time_in_ms: i64,
        is_beat: bool,
        pixel_format: AvsPixelFormat,
    ) -> Result<(), AvsError> {
        let (width, height) = (framebuffer.width, framebuffer.height);
        let framebuffer: *mut c_void = match &mut framebuffer.data {
            AvsBufferData::Vec(ref mut vec) => vec.as_mut_ptr() as *mut c_void,
            AvsBufferData::Mmap(ref mut mmap) => mmap.as_mut_ptr() as *mut c_void,
            AvsBufferData::Raw(ptr) => *ptr,
        };
        if !unsafe {
            avs_render_frame(
                self.handle,
                framebuffer,
                width,
                height,
                time_in_ms,
                is_beat,
                pixel_format.to_value(),
            )
        } {
            return Err(self.error("render_frame"));
        }
        Ok(())
    }

    pub fn audio_set(
        &self,
        audio_data: (Vec<f32>, Vec<f32>),
        samples_per_second: usize,
        end_time_in_samples: i64,
    ) -> Result<i32, AvsError> {
        if audio_data.0.len() != audio_data.1.len() {
            return Err(AvsError::with_str(
                "audio_set: audio_data[0] and audio_data[1] must have the same length",
            ));
        }
        let millis_audio_available_before = !unsafe {
            avs_audio_set(
                self.handle,
                audio_data.0.as_ptr(),
                audio_data.1.as_ptr(),
                audio_data.0.len(),
                samples_per_second,
                end_time_in_samples,
            )
        };
        match millis_audio_available_before {
            i32::MAX => {
                Err(AvsError::with_str("audio_set: Audio data too long for buffer"))
            }
            -1 => Err(AvsError::with_str("audio_set: No external audio configured")),
            _ => Ok(millis_audio_available_before),
        }
    }

    fn audio_device_count(&self) -> Result<usize, AvsError> {
        match unsafe { avs_audio_device_count(self.handle) } {
            ..=-1 => Err(self.error("audio_device_count")),
            count => Ok(count as usize),
        }
    }

    pub fn audio_devices(&self) -> Result<Vec<String>, AvsError> {
        const MAX_AUDIO_DEVICES_SAFETY: usize = 65535usize;
        let count = self.audio_device_count()?;
        let names_c_list = unsafe { avs_audio_device_names(self.handle) };
        if names_c_list.is_null() {
            return Err(self.error("audio_devices"));
        }
        let mut names = Vec::with_capacity(count);
        for i in 0..=MAX_AUDIO_DEVICES_SAFETY {
            let name_ptr = unsafe { *names_c_list.add(i) };
            if name_ptr.is_null() {
                break;
            }
            names.push(str_from_c_lossy_or_empty(name_ptr));
        }
        Ok(names)
    }

    pub fn audio_device_set(&self, device: &String) -> Result<(), AvsError> {
        let device_names = self.audio_devices()?;
        let device_index: i32 = device_names.iter().position(|d| d == device).ok_or(
            AvsError::with_string(format!(
                "audio_device_set: Device \"{}\" not found",
                device
            )),
        )? as i32;
        if !unsafe { avs_audio_device_set(self.handle, device_index) } {
            return Err(self.error("audio_device_set"));
        }
        Ok(())
    }

    pub fn input_key_set(&self, key: u32, state: bool) -> Result<(), AvsError> {
        if !unsafe { avs_input_key_set(self.handle, key, state) } {
            return Err(self.error("input_key_set"));
        }
        Ok(())
    }

    pub fn input_mouse_pos_set(&self, x: f64, y: f64) -> Result<(), AvsError> {
        if !unsafe { avs_input_mouse_pos_set(self.handle, x, y) } {
            return Err(self.error("input_mouse_pos_set"));
        }
        Ok(())
    }

    pub fn input_mouse_button_set(
        &self,
        button: u32,
        state: bool,
    ) -> Result<(), AvsError> {
        if !unsafe { avs_input_mouse_button_set(self.handle, button, state) } {
            return Err(self.error("input_mouse_button_set"));
        }
        Ok(())
    }

    pub fn preset_load(&mut self, file_path: &Path) -> Result<(), AvsError> {
        let file_path_cstr = CString::new(
            file_path
                .to_str()
                .ok_or(AvsError::with_str("preset_load: path has invalid encoding"))?,
        )
        .map_err(|_| AvsError::with_str("preset_load: null byte in path"))?;
        if !unsafe { avs_preset_load(self.handle, file_path_cstr.as_ptr()) } {
            return Err(self.error("preset_load"));
        }
        self.reload_component_tree()?;
        Ok(())
    }

    pub fn preset_set(&mut self, preset: &str) -> Result<(), AvsError> {
        let preset_cstr = CString::new(preset)
            .map_err(|_| AvsError::with_str("preset_set: null byte in preset JSON"))?;
        if !unsafe { avs_preset_set(self.handle, preset_cstr.as_ptr()) } {
            return Err(self.error("preset_set"));
        }
        self.reload_component_tree()?;
        Ok(())
    }

    pub fn preset_save(
        &self,
        file_path: &Path,
        as_remix: bool,
        indent: bool,
    ) -> Result<(), AvsError> {
        let file_path_cstr = CString::new(
            file_path
                .to_str()
                .ok_or(AvsError::with_str("preset_save: path has invalid encoding"))?,
        )
        .map_err(|_| AvsError::with_str("preset_save: null byte in path"))?;
        if !unsafe {
            avs_preset_save(self.handle, file_path_cstr.as_ptr(), as_remix, indent)
        } {
            return Err(self.error("preset_save"));
        }
        Ok(())
    }

    pub fn preset_get(&self, as_remix: bool, indent: bool) -> Result<String, AvsError> {
        let preset_str_ptr = unsafe { avs_preset_get(self.handle, as_remix, indent) };
        if preset_str_ptr.is_null() {
            return Err(self.error("preset_get"));
        }
        Ok(str_from_c_lossy_or_empty(preset_str_ptr))
    }

    pub fn preset_set_legacy(&mut self, preset: &[u8]) -> Result<(), AvsError> {
        if !unsafe { avs_preset_set_legacy(self.handle, preset.as_ptr(), preset.len()) }
        {
            return Err(self.error("preset_set_legacy"));
        }
        self.reload_component_tree()?;
        Ok(())
    }

    pub fn preset_get_legacy(&self) -> Result<Vec<u8>, AvsError> {
        let mut length_out = 0usize;
        let preset_ptr = unsafe {
            avs_preset_get_legacy(self.handle, std::ptr::from_mut(&mut length_out))
        };
        let slice = unsafe { slice::from_raw_parts(preset_ptr, length_out) };
        Ok(slice.into())
    }

    pub fn preset_save_legacy(&self, file_path: &Path) -> Result<(), AvsError> {
        let file_path_cstr = CString::new(
            file_path
                .to_str()
                .ok_or(AvsError::with_str("preset_load: path has invalid encoding"))?,
        )
        .map_err(|_| AvsError::with_str("preset_load: null byte in path"))?;
        if !unsafe { avs_preset_save_legacy(self.handle, file_path_cstr.as_ptr()) } {
            return Err(self.error("preset_save_legacy"));
        }
        Ok(())
    }

    pub fn preset_format_schema() -> Result<String, AvsError> {
        let schema_str_ptr = unsafe { avs_preset_format_schema() };
        if schema_str_ptr.is_null() {
            return Err(Avs::default().error("preset_format_schema"));
        }
        Ok(str_from_c_lossy_or_empty(schema_str_ptr))
    }

    pub fn lib_version() -> AVS_Version {
        unsafe { avs_version() }
    }

    fn error(&self, prefix: &str) -> AvsError {
        let error_ptr = unsafe { avs_error_str(self.handle) };
        if error_ptr.is_null() {
            return AvsError::with_str("unknown AVS error");
        }
        // Safety: `error_ptr` is not null anymore here. What else?
        let err_msg = &unsafe { CStr::from_ptr(error_ptr) }.to_string_lossy();
        if err_msg.is_empty() {
            AvsError::with_string(prefix.to_string() + ": No error")
        } else {
            AvsError::with_string(prefix.to_string() + ": " + err_msg)
        }
    }
}

impl Drop for Avs {
    fn drop(&mut self) {
        unsafe { avs_free(self.handle) }
    }
}

unsafe impl Send for Avs {}

pub struct AvsFramebuffer<'a> {
    pub data: &'a mut AvsBufferData<'a>,
    pub width: usize,
    pub height: usize,
}

pub enum AvsBufferData<'a> {
    Vec(&'a mut Vec<u32>),
    Mmap(MmapMut),
    Raw(*mut c_void),
}

impl AvsFramebuffer<'_> {
    pub fn resize(&mut self, width: usize, height: usize) -> Result<(), AvsError> {
        match *self.data {
            AvsBufferData::Vec(ref mut v) => {
                **v = vec![0; width * height];
                self.width = width;
                self.height = height;
            }
            _ => {
                return Err(AvsError::with_str(
                    "can only resize Vec<u32>-typed framebuffers",
                ))
            }
        };
        Ok(())
    }
}

pub struct AvsError {
    pub message: String,
}
impl AvsError {
    pub fn with_string(message: String) -> AvsError {
        AvsError { message }
    }
    pub fn with_str(message: &str) -> AvsError {
        AvsError { message: message.to_string() }
    }
}
impl fmt::Display for AvsError {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(f, "{}", self.message)
    }
}
impl fmt::Debug for AvsError {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(f, "{}", self.message)
    }
}
impl Error for AvsError {}

// === Editor API ===

#[derive(Debug, Default, FromCEnum, PartialEq)]
#[repr(i32)]
pub enum AvsParameterType {
    Invalid = AVS_PARAM_INVALID,
    List = AVS_PARAM_LIST,
    Action = AVS_PARAM_ACTION,
    #[default]
    Bool = AVS_PARAM_BOOL,
    Int = AVS_PARAM_INT,
    Float = AVS_PARAM_FLOAT,
    Color = AVS_PARAM_COLOR,
    String = AVS_PARAM_STRING,
    Select = AVS_PARAM_SELECT,
    Resource = AVS_PARAM_RESOURCE,
    IntArray = AVS_PARAM_INT_ARRAY,
    FloatArray = AVS_PARAM_FLOAT_ARRAY,
    ColorArray = AVS_PARAM_COLOR_ARRAY,
}

#[derive(Default)]
pub struct AvsEffect {
    handle: AVS_Effect_Handle,
    pub group: String,
    pub name: String,
    pub help: String,
    pub parameters: Vec<AvsParameter>,
    pub can_have_child_components: bool,
    pub is_user_creatable: bool,
}

type AvsEffectLibrary = BTreeMap<AVS_Effect_Handle, Rc<AvsEffect>>;

pub struct AvsParameter {
    handle: AVS_Parameter_Handle,
    pub r#type: AvsParameterType,
    pub name: String,
    pub description: Option<String>,
    pub is_global: bool,
    pub int_min: i64,
    pub int_max: i64,
    pub float_min: f64,
    pub float_max: f64,
    pub options: Option<Vec<String>>,
    pub children: Vec<AvsParameter>,
    pub children_min: u32,
    pub children_max: u32,
}

impl Avs {
    fn effect_from_handle(
        &self,
        effect_handle: AVS_Effect_Handle,
    ) -> Option<Rc<AvsEffect>> {
        for (handle, effect) in &self.effect_library {
            if *handle == effect_handle {
                return Some(effect.clone());
            }
        }
        None
    }

    pub fn effect_from_name(&self, effect_name: &str) -> Option<Rc<AvsEffect>> {
        for (_, effect) in &self.effect_library {
            if effect.name == effect_name {
                return Some(effect.clone());
            }
        }
        None
    }

    fn effect_library(avs_handle: AVS_Handle) -> Result<AvsEffectLibrary, AvsError> {
        let mut length_out = 0u32;
        let library_ptr = unsafe { avs_effect_library(avs_handle, &mut length_out) };
        if library_ptr.is_null() {
            return Err(Avs::default().error("effect_library"));
        }
        let handles_slice =
            unsafe { slice::from_raw_parts(library_ptr, length_out as usize) };
        let mut effect_library = BTreeMap::new();
        for effect_handle in handles_slice {
            match Avs::effect_info(avs_handle, *effect_handle) {
                Ok(effect) => {
                    effect_library.insert(*effect_handle, Rc::new(effect));
                }
                Err(err) => println!("skipping effect: {err}"),
            }
        }
        Ok(effect_library)
    }

    fn effect_info(
        avs_handle: AVS_Handle,
        effect_handle: AVS_Effect_Handle,
    ) -> Result<AvsEffect, AvsError> {
        let mut out_struct = AVS_Effect_Info {
            group: std::ptr::null(),
            name: std::ptr::null(),
            help: std::ptr::null(),
            parameters_length: 0u32,
            parameters: std::ptr::null(),
            can_have_child_components: false,
            is_user_creatable: false,
        };
        if !unsafe { avs_effect_info(avs_handle, effect_handle, &mut out_struct) } {
            return Err(Avs::default().error("effect_info"));
        }

        Ok(AvsEffect {
            handle: effect_handle,
            group: str_from_c_lossy_or_empty(out_struct.group),
            name: str_from_c_lossy_or_empty(out_struct.name),
            help: str_from_c_lossy_or_empty(out_struct.help),
            parameters: Avs::parameter_list(
                avs_handle,
                effect_handle,
                out_struct.parameters,
                out_struct.parameters_length as usize,
            )
            .map_err(|e| {
                AvsError::with_string(format!(
                    "{e}: {} (0x{effect_handle:08x})",
                    str_from_c_lossy_or_empty(out_struct.name)
                ))
            })?,
            can_have_child_components: out_struct.can_have_child_components,
            is_user_creatable: out_struct.is_user_creatable,
        })
    }

    fn parameter_list(
        avs_handle: AVS_Handle,
        effect_handle: AVS_Effect_Handle,
        parameter_handles: *const AVS_Parameter_Handle,
        length: usize,
    ) -> Result<Vec<AvsParameter>, AvsError> {
        if parameter_handles.is_null() || length == 0 {
            return Ok(Vec::new());
        }
        let parameters: Result<Vec<_>, _> =
            unsafe { slice::from_raw_parts(parameter_handles, length) }
                .iter()
                .enumerate()
                .map(|(i, &parameter_handle)| {
                    Avs::parameter_info(
                        avs_handle,
                        effect_handle,
                        parameter_handle,
                        i + 1,
                    )
                })
                .collect();
        Ok(parameters?)
    }

    fn parameter_info(
        avs_handle: AVS_Handle,
        effect_handle: AVS_Effect_Handle,
        parameter_handle: AVS_Parameter_Handle,
        parameter_index_1: usize,
    ) -> Result<AvsParameter, AvsError> {
        let mut out_struct = AVS_Parameter_Info {
            type_: AVS_PARAM_INVALID,
            name: std::ptr::null(),
            description: std::ptr::null(),
            is_global: false,
            int_min: 0,
            int_max: 0,
            float_min: 0.0,
            float_max: 0.0,
            options_length: 0,
            options: std::ptr::null(),
            children_length: 0,
            children_length_min: 0,
            children_length_max: 0,
            children: std::ptr::null(),
        };
        unsafe {
            avs_parameter_info(
                avs_handle,
                effect_handle,
                parameter_handle,
                &mut out_struct,
            )
        };
        let mut options: Option<Vec<_>> = None;
        let r#type = AvsParameterType::from_value(out_struct.type_)
            .ok_or(AvsError::with_str("parameter_info: invalid parameter type"))?;
        if (r#type == AvsParameterType::Select || r#type == AvsParameterType::Resource)
            && !out_struct.options.is_null()
            && out_struct.options_length > 0
        {
            options = Some(
                unsafe {
                    slice::from_raw_parts(
                        out_struct.options,
                        out_struct.options_length as usize,
                    )
                }
                .iter()
                .map(|&s| str_from_c_lossy_or_empty(s))
                .collect(),
            );
        };
        Ok(AvsParameter {
            handle: parameter_handle,
            r#type,
            name: str_from_c_lossy(out_struct.name).ok_or(AvsError::with_string(
                format!("parameter {parameter_index_1}: missing name"),
            ))?,
            description: str_from_c_lossy(out_struct.description),
            is_global: out_struct.is_global,
            int_min: out_struct.int_min,
            int_max: out_struct.int_max,
            float_min: out_struct.float_min,
            float_max: out_struct.float_max,
            options,
            children: Avs::parameter_list(
                avs_handle,
                effect_handle,
                out_struct.children,
                out_struct.children_length as usize,
            )
            .map_err(|e| AvsError::with_string(format!("{parameter_index_1}: {e}")))?,
            children_min: out_struct.children_length_min,
            children_max: out_struct.children_length_max,
        })
    }
}

#[derive(Debug, Default, FromCEnum, PartialEq)]
#[repr(i32)]
pub enum AvsComponentPosition {
    DontCare = AVS_COMPONENT_POSITION_DONTCARE,
    Before = AVS_COMPONENT_POSITION_BEFORE,
    #[default]
    After = AVS_COMPONENT_POSITION_AFTER,
    Child = AVS_COMPONENT_POSITION_CHILD,
}

#[derive(Default)]
pub struct AvsComponent {
    pub(crate) handle: AVS_Component_Handle,
    pub(crate) effect: AVS_Effect_Handle,
    pub enabled: bool,
    pub comment: String,
}

impl PartialEq for AvsComponent {
    fn eq(&self, other: &Self) -> bool {
        self.handle == other.handle
    }
}

impl Avs {
    fn reload_component_tree(&mut self) -> Result<(), AvsError> {
        let root_handle = unsafe { avs_component_root(self.handle) };
        if root_handle == 0 {
            return Err(self.error("component_root"));
        }
        self.preset.clear();
        self.into_component_tree(None, self.component_from_handle(root_handle)?)?;
        Ok(())
    }

    fn component_from_handle(
        &self,
        handle: AVS_Component_Handle,
    ) -> Result<AvsComponent, AvsError> {
        let effect = self.component_effect(handle)?;
        let properties = self.component_properties_get(handle)?;
        Ok(AvsComponent {
            handle,
            effect: effect.handle,
            enabled: properties.enabled,
            comment: str_from_c_lossy_or_empty(properties.comment),
        })
    }

    fn component_effect(
        &self,
        handle: AVS_Component_Handle,
    ) -> Result<Rc<AvsEffect>, AvsError> {
        let effect_handle = unsafe { avs_component_effect(self.handle, handle) };
        if effect_handle == 0 {
            return Err(self.error("component_effect"));
        }
        self.effect_from_handle(effect_handle).ok_or(AvsError::with_string(format!(
            "component_effect: Effect not found for handle {}",
            handle
        )))
    }

    fn component_properties_get(
        &self,
        handle: AVS_Component_Handle,
    ) -> Result<AVS_Component_Properties, AvsError> {
        let mut out_struct =
            AVS_Component_Properties { enabled: false, comment: std::ptr::null() };
        if !unsafe {
            avs_component_properties_get(self.handle, handle, &mut out_struct)
        } {
            return Err(self.error("component_properties_get"));
        }
        Ok(out_struct)
    }

    fn into_component_tree(
        &mut self,
        parent: Option<usize>,
        component: AvsComponent,
    ) -> Result<(), AvsError> {
        let mut length_out = 0u32;
        let child_handles = unsafe {
            avs_component_children(self.handle, component.handle, &mut length_out)
        };
        let id = self.preset.insert(parent, component);
        if length_out == 0 {
            return Ok(());
        }
        if child_handles.is_null() {
            return Err(self.error("into_component_tree"));
        }
        let handles_slice =
            unsafe { slice::from_raw_parts(child_handles, length_out as usize) };
        for child_handle in handles_slice {
            // println!("child_handle: 0x{:08x}", *child_handle);
            self.into_component_tree(id, self.component_from_handle(*child_handle)?)?;
        }
        Ok(())
    }

    pub fn component_name(&self, component: &AvsComponent) -> String {
        self.effect_library
            .get(&component.effect)
            .map(|e| e.name.clone())
            .unwrap_or("?".to_string())
    }

    pub fn print_component_tree(&self) -> Vec<String> {
        self.preset
            .iter()
            .map(|(depth, c)| {
                let name = if c.enabled {
                    self.component_name(c)
                } else {
                    format!("\x1b[90m{}\x1b[0m", self.component_name(c))
                };
                format!("{}{name}", "  ".repeat(depth))
            })
            .collect()
    }

    pub fn component_create(
        &mut self,
        effect_handle: AVS_Effect_Handle,
        relative_to: &AvsComponent,
        direction: AvsComponentPosition,
    ) -> Result<(), AvsError> {
        let handle = unsafe {
            avs_component_create(
                self.handle,
                effect_handle,
                relative_to.handle,
                direction.to_value(),
            )
        };
        if handle == 0 {
            return Err(self.error("component_create"));
        }
        self.reload_component_tree()?;
        Ok(())
    }

    pub fn create(
        &mut self,
        effect_name: &str,
        relative_to: &AvsComponent,
        direction: AvsComponentPosition,
    ) -> Result<(), AvsError> {
        match self.effect_from_name(effect_name) {
            Some(effect) => {
                self.component_create(effect.handle, &relative_to, direction)
            }
            None => Err(AvsError::with_string(format!(
                "create: Effect \"{}\" not found",
                effect_name
            ))),
        }
    }

    pub fn first_of(&self, effect_name: &str) -> Option<Rc<&AvsComponent>> {
        let effect = self.effect_from_name(effect_name)?;
        self.preset
            .iter()
            .find(|(_, c)| c.effect == effect.handle)
            .map(|(_, c)| Rc::new(c))
    }
}

impl AvsComponent {}

fn str_from_c_lossy_or_empty(cstr: *const i8) -> String {
    str_from_c_lossy(cstr).unwrap_or(String::new())
}

fn str_from_c_lossy(cstr: *const i8) -> Option<String> {
    if !cstr.is_null() {
        Some(unsafe { CStr::from_ptr(cstr) }.to_string_lossy().into_owned())
    } else {
        None
    }
}
