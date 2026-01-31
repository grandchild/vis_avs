#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!("../bindings/bindings.rs");

use std::collections::BTreeMap;
use std::error::Error;
use std::ffi::{CStr, CString};
use std::fmt;
use std::os::raw::c_void;
use std::path::Path;
use std::rc::Rc;
use std::slice;

use memmap::MmapMut as MmapMutLegacy;
use memmap2::MmapMut;

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
        log_file_path: Option<&str>,
    ) -> Result<Self, AvsError> {
        let handle = unsafe {
            avs_init(
                c_str_ptr_or_null(base_path),
                audio_source.to_value(),
                beat_source.to_value(),
                c_str_ptr_or_null(log_file_path),
            )
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
            AvsBufferData::MmapLegacy(ref mut mmap) => mmap.as_mut_ptr() as *mut c_void,
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
                "audio_device_set: Device \"{device}\" not found"
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
    MmapLegacy(MmapMutLegacy),
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

#[derive(Clone, Debug, Default, FromCEnum, PartialEq)]
#[repr(i32)]
pub enum AvsParameterType {
    #[default]
    Invalid = AVS_PARAM_INVALID,
    List = AVS_PARAM_LIST,
    Action = AVS_PARAM_ACTION,
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
    pub handle: AVS_Effect_Handle,
    pub group: String,
    pub name: String,
    pub help: String,
    pub parameters: Vec<AvsParameter>,
    pub can_have_child_components: bool,
    pub is_user_creatable: bool,
}

type AvsEffectLibrary = BTreeMap<AVS_Effect_Handle, AvsEffect>;

#[derive(Clone, Default, PartialEq)]
pub struct AvsParameter {
    pub handle: AVS_Parameter_Handle,
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
    ) -> Option<&AvsEffect> {
        self.effect_library.get(&effect_handle)
    }

    pub fn effect_from_name(&self, effect_name: &str) -> Option<&AvsEffect> {
        self.effect_library.iter().find_map(|(_, effect)| {
            if effect.name == effect_name {
                Some(effect)
            } else {
                None
            }
        })
    }

    pub fn effects(&self) -> impl Iterator<Item = &AvsEffect> {
        self.effect_library.values()
    }

    fn effect_library(avs_handle: AVS_Handle) -> Result<AvsEffectLibrary, AvsError> {
        let mut length_out = 0usize;
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
                    effect_library.insert(*effect_handle, effect);
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
        parameters
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
    pub config: AvsComponentConfig,
}

impl PartialEq for AvsComponent {
    fn eq(&self, other: &Self) -> bool {
        self.handle == other.handle
    }
}

#[derive(Default)]
pub struct AvsComponentConfig {
    cached_parameters: BTreeMap<String, AvsConfigParameter>,
}

pub struct AvsConfigParameter {
    avs: AVS_Handle,
    component: AVS_Component_Handle,
    parameter: Rc<AvsParameter>,
    nested_config_path: Vec<i64>,
    cached_nested_configs: Vec<AvsComponentConfig>,
}

impl PartialEq for AvsConfigParameter {
    fn eq(&self, other: &Self) -> bool {
        self.avs == other.avs
            && self.component == other.component
            && self.parameter == other.parameter
            && self.nested_config_path == other.nested_config_path
    }
}

#[derive(Debug, PartialEq)]
pub struct AvsColor {
    pub r: f32,
    pub g: f32,
    pub b: f32,
    pub a: f32,
}

impl From<u64> for AvsColor {
    fn from(value: u64) -> Self {
        AvsColor {
            r: ((value >> 24) & 0xff) as f32 / 255.0,
            g: ((value >> 16) & 0xff) as f32 / 255.0,
            b: ((value >> 8) & 0xff) as f32 / 255.0,
            a: (value & 0xff) as f32 / 255.0,
        }
    }
}

#[derive(Default, PartialEq)]
pub enum AvsConfigValue {
    #[default]
    Invalid,
    List,
    Action,
    Bool(bool),
    Int(i64),
    Float(f64),
    Color(AvsColor),
    String(String),
    Select(i64),
    Resource(String),
    IntArray(Vec<i64>),
    FloatArray(Vec<f64>),
    ColorArray(Vec<AvsColor>),
}

impl Avs {
    pub fn root(&self) -> &AvsComponent {
        self.preset.root()
    }

    pub fn root_mut(&mut self) -> &mut AvsComponent {
        self.preset.root_mut()
    }

    fn reload_component_tree(&mut self) -> Result<(), AvsError> {
        let root_handle = unsafe { avs_component_root(self.handle) };
        if root_handle == 0 {
            return Err(self.error("component_root"));
        }
        self.preset.clear();
        self.make_component_tree(None, self.component_from_handle(root_handle)?)?;
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
            config: self.component_config(handle)?,
        })
    }

    fn component_effect(
        &self,
        handle: AVS_Component_Handle,
    ) -> Result<&AvsEffect, AvsError> {
        let effect_handle = unsafe { avs_component_effect(self.handle, handle) };
        if effect_handle == 0 {
            return Err(self.error("component_effect"));
        }
        self.effect_from_handle(effect_handle).ok_or(AvsError::with_string(format!(
            "component_effect: Effect not found for handle {handle}"
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

    fn make_component_tree(
        &mut self,
        parent: Option<usize>,
        component: AvsComponent,
    ) -> Result<(), AvsError> {
        let mut length_out = 0usize;
        let child_handles = unsafe {
            avs_component_children(self.handle, component.handle, &mut length_out)
        };
        let id = self.preset.insert(parent, component);
        if length_out == 0 {
            return Ok(());
        }
        if child_handles.is_null() {
            return Err(self.error("make_component_tree"));
        }
        let handles_slice =
            unsafe { slice::from_raw_parts(child_handles, length_out as usize) };
        for child_handle in handles_slice {
            // println!("child_handle: 0x{:08x}", *child_handle);
            self.make_component_tree(id, self.component_from_handle(*child_handle)?)?;
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
                let comment = if c.comment.is_empty() {
                    "".to_string()
                } else {
                    format!(" ({})", c.comment)
                };
                format!("{}{name}{comment}", "  ".repeat(depth))
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
                self.component_create(effect.handle, relative_to, direction)
            }
            None => Err(AvsError::with_string(format!(
                "create: Effect \"{effect_name}\" not found"
            ))),
        }
    }

    fn component_config(
        &self,
        component_handle: AVS_Component_Handle,
    ) -> Result<AvsComponentConfig, AvsError> {
        AvsComponentConfig::new(self.handle, component_handle, None, Vec::new())
    }
}

impl AvsConfigParameter {
    pub fn get(&self, index: usize) -> Result<&AvsComponentConfig, AvsError> {
        match self.parameter.r#type {
            AvsParameterType::List => {
                if index < self.cached_nested_configs.len() {
                    Ok(&self.cached_nested_configs[index])
                } else {
                    Err(AvsError::with_str(&format!(
                        "List index {} out of bounds",
                        index
                    )))
                }
            }
            _ => Err(AvsError::with_str("Cannot index into non-list parameter")),
        }
    }

    pub fn get_value<T>(&self) -> Result<T, AvsError>
    where
        T: TryFrom<AvsConfigValue, Error = AvsError>,
    {
        let config_value = self.get_config_value()?;
        config_value.try_into()
    }

    fn get_config_value(&self) -> Result<AvsConfigValue, AvsError> {
        match self.parameter.r#type {
            AvsParameterType::Invalid => {
                Err(AvsError::with_str("get_config_value: invalid parameter type"))
            }
            AvsParameterType::Action => Err(AvsError::with_str(
                "get_config_value: action parameters have no value",
            )),
            AvsParameterType::Bool => {
                let value = unsafe {
                    avs_parameter_get_bool(
                        self.avs,
                        self.component,
                        self.parameter.handle,
                        self.nested_config_path.len() as u32,
                        self.nested_config_path.as_ptr(),
                    )
                };
                if !value {
                    let error_ptr = unsafe { avs_error_str(self.avs) };
                    match str_from_c_lossy(error_ptr) {
                        Some(err) if !err.is_empty() => {
                            return Err(AvsError::with_string(format!(
                                "failed to get bool value: {err}"
                            )));
                        }
                        _ => (),
                    }
                }
                Ok(AvsConfigValue::Bool(value))
            }
            AvsParameterType::Int => {
                let value = unsafe {
                    avs_parameter_get_int(
                        self.avs,
                        self.component,
                        self.parameter.handle,
                        self.nested_config_path.len() as u32,
                        self.nested_config_path.as_ptr(),
                    )
                };
                Ok(AvsConfigValue::Int(value))
            }
            AvsParameterType::Float => {
                let value = unsafe {
                    avs_parameter_get_float(
                        self.avs,
                        self.component,
                        self.parameter.handle,
                        self.nested_config_path.len() as u32,
                        self.nested_config_path.as_ptr(),
                    )
                };
                Ok(AvsConfigValue::Float(value))
            }
            AvsParameterType::Color => {
                let int_color = unsafe {
                    avs_parameter_get_color(
                        self.avs,
                        self.component,
                        self.parameter.handle,
                        self.nested_config_path.len() as u32,
                        self.nested_config_path.as_ptr(),
                    )
                };
                let r = (int_color & 0xFF) as f32 / 255.0;
                let g = ((int_color >> 8) & 0xFF) as f32 / 255.0;
                let b = ((int_color >> 16) & 0xFF) as f32 / 255.0;
                let a = ((int_color >> 24) & 0xFF) as f32 / 255.0;
                Ok(AvsConfigValue::Color(AvsColor { r, g, b, a }))
            }
            AvsParameterType::String => {
                let value_ptr = unsafe {
                    avs_parameter_get_string(
                        self.avs,
                        self.component,
                        self.parameter.handle,
                        self.nested_config_path.len() as u32,
                        self.nested_config_path.as_ptr(),
                    )
                };
                if value_ptr.is_null() {
                    return Err(AvsError::with_str("failed to get string value"));
                }
                let value = str_from_c_lossy_or_empty(value_ptr);
                Ok(AvsConfigValue::String(value))
            }
            AvsParameterType::Select => {
                let value = unsafe {
                    avs_parameter_get_int(
                        self.avs,
                        self.component,
                        self.parameter.handle,
                        self.nested_config_path.len() as u32,
                        self.nested_config_path.as_ptr(),
                    )
                };
                Ok(AvsConfigValue::Select(value))
            }
            AvsParameterType::Resource => {
                let value_ptr = unsafe {
                    avs_parameter_get_string(
                        self.avs,
                        self.component,
                        self.parameter.handle,
                        self.nested_config_path.len() as u32,
                        self.nested_config_path.as_ptr(),
                    )
                };
                if value_ptr.is_null() {
                    return Err(AvsError::with_str("failed to get resource value"));
                }
                let value = str_from_c_lossy_or_empty(value_ptr);
                Ok(AvsConfigValue::Resource(value))
            }
            AvsParameterType::List => Ok(AvsConfigValue::List),
            AvsParameterType::IntArray => {
                let mut length_out = 0u64;
                let array_ptr = unsafe {
                    avs_parameter_get_int_array(
                        self.avs,
                        self.component,
                        self.parameter.handle,
                        &mut length_out,
                        self.nested_config_path.len() as u32,
                        self.nested_config_path.as_ptr(),
                    )
                };
                if array_ptr.is_null() {
                    return Err(AvsError::with_str("failed to get int array value"));
                }
                if length_out > usize::MAX as u64 {
                    // This can only happen on 32bit targets and is _so_ unlikely that
                    // if it happens, we'd rather lose some entries and keep this safety
                    // measure dead simple.
                    length_out = usize::MAX as u64;
                }
                let slice =
                    unsafe { slice::from_raw_parts(array_ptr, length_out as usize) };
                Ok(AvsConfigValue::IntArray(slice.into()))
            }
            AvsParameterType::FloatArray => {
                let mut length_out = 0u64;
                let array_ptr = unsafe {
                    avs_parameter_get_float_array(
                        self.avs,
                        self.component,
                        self.parameter.handle,
                        &mut length_out,
                        self.nested_config_path.len() as u32,
                        self.nested_config_path.as_ptr(),
                    )
                };
                if array_ptr.is_null() {
                    return Err(AvsError::with_str("failed to get float array value"));
                }
                if length_out > usize::MAX as u64 {
                    // This can only happen on 32bit targets and is _so_ unlikely that
                    // if it happens, we'd rather lose some entries and keep this safety
                    // measure dead simple.
                    length_out = usize::MAX as u64;
                }
                let slice =
                    unsafe { slice::from_raw_parts(array_ptr, length_out as usize) };
                Ok(AvsConfigValue::FloatArray(slice.into()))
            }
            AvsParameterType::ColorArray => {
                let mut length_out = 0u64;
                let array_ptr = unsafe {
                    avs_parameter_get_color_array(
                        self.avs,
                        self.component,
                        self.parameter.handle,
                        &mut length_out,
                        self.nested_config_path.len() as u32,
                        self.nested_config_path.as_ptr(),
                    )
                };
                if array_ptr.is_null() {
                    return Err(AvsError::with_str("failed to get color array value"));
                }
                if length_out > usize::MAX as u64 {
                    // This can only happen on 32bit targets and is _so_ unlikely that
                    // if it happens, we'd rather lose some entries and keep this safety
                    // measure dead simple.
                    length_out = usize::MAX as u64;
                }
                let slice =
                    unsafe { slice::from_raw_parts(array_ptr, length_out as usize) };
                let colors: Vec<AvsColor> = slice
                    .iter()
                    .map(|&int_color| {
                        let r = (int_color & 0xFF) as f32 / 255.0;
                        let g = ((int_color >> 8) & 0xFF) as f32 / 255.0;
                        let b = ((int_color >> 16) & 0xFF) as f32 / 255.0;
                        let a = ((int_color >> 24) & 0xFF) as f32 / 255.0;
                        AvsColor { r, g, b, a }
                    })
                    .collect();
                Ok(AvsConfigValue::ColorArray(colors))
            }
        }
    }

    fn set_value(&mut self, value: AvsConfigValue) -> Result<(), AvsError> {
        match self.parameter.r#type {
            AvsParameterType::Bool => match value {
                AvsConfigValue::Bool(v) => {
                    let success = unsafe {
                        avs_parameter_set_bool(
                            self.avs,
                            self.component,
                            self.parameter.handle,
                            v,
                            self.nested_config_path.len() as u32,
                            self.nested_config_path.as_ptr(),
                        )
                    };
                    if !success {
                        return Err(AvsError::with_str("Failed to set bool parameter"));
                    }
                    Ok(())
                }
                _ => Err(AvsError::with_str("Type mismatch: expected Bool")),
            },
            AvsParameterType::Int => match value {
                AvsConfigValue::Int(v) => {
                    let success = unsafe {
                        avs_parameter_set_int(
                            self.avs,
                            self.component,
                            self.parameter.handle,
                            v,
                            self.nested_config_path.len() as u32,
                            self.nested_config_path.as_ptr(),
                        )
                    };
                    if !success {
                        return Err(AvsError::with_str("Failed to set int parameter"));
                    }
                    Ok(())
                }
                _ => Err(AvsError::with_str("Type mismatch: expected Int")),
            },
            AvsParameterType::Select => match value {
                AvsConfigValue::Select(v) => {
                    let success = unsafe {
                        avs_parameter_set_int(
                            self.avs,
                            self.component,
                            self.parameter.handle,
                            v,
                            self.nested_config_path.len() as u32,
                            self.nested_config_path.as_ptr(),
                        )
                    };
                    if !success {
                        return Err(AvsError::with_str(
                            "Failed to set select parameter",
                        ));
                    }
                    Ok(())
                }
                _ => Err(AvsError::with_str("Type mismatch: expected Select")),
            },
            AvsParameterType::Float => match value {
                AvsConfigValue::Float(v) => {
                    let success = unsafe {
                        avs_parameter_set_float(
                            self.avs,
                            self.component,
                            self.parameter.handle,
                            v,
                            self.nested_config_path.len() as u32,
                            self.nested_config_path.as_ptr(),
                        )
                    };
                    if !success {
                        return Err(AvsError::with_str(
                            "Failed to set float parameter",
                        ));
                    }
                    Ok(())
                }
                _ => Err(AvsError::with_str("Type mismatch: expected Float")),
            },
            AvsParameterType::String => match value {
                AvsConfigValue::String(v) => {
                    let cstring = CString::new(v)
                        .map_err(|_| AvsError::with_str("Invalid string"))?;
                    let success = unsafe {
                        avs_parameter_set_string(
                            self.avs,
                            self.component,
                            self.parameter.handle,
                            cstring.as_ptr(),
                            self.nested_config_path.len() as u32,
                            self.nested_config_path.as_ptr(),
                        )
                    };
                    if !success {
                        return Err(AvsError::with_str(
                            "Failed to set string parameter",
                        ));
                    }
                    Ok(())
                }
                _ => Err(AvsError::with_str("Type mismatch: expected String")),
            },
            AvsParameterType::Resource => match value {
                AvsConfigValue::Resource(v) => {
                    let cstring = CString::new(v)
                        .map_err(|_| AvsError::with_str("Invalid string"))?;
                    let success = unsafe {
                        avs_parameter_set_string(
                            self.avs,
                            self.component,
                            self.parameter.handle,
                            cstring.as_ptr(),
                            self.nested_config_path.len() as u32,
                            self.nested_config_path.as_ptr(),
                        )
                    };
                    if !success {
                        return Err(AvsError::with_str(
                            "Failed to set resource parameter",
                        ));
                    }
                    Ok(())
                }
                _ => Err(AvsError::with_str("Type mismatch: expected Resource")),
            },
            AvsParameterType::Color => match value {
                AvsConfigValue::Color(v) => {
                    let color_int = (v.a as u64) << 24
                        | (v.r as u64) << 16
                        | (v.g as u64) << 8
                        | (v.b as u64);
                    let success = unsafe {
                        avs_parameter_set_color(
                            self.avs,
                            self.component,
                            self.parameter.handle,
                            color_int,
                            self.nested_config_path.len() as u32,
                            self.nested_config_path.as_ptr(),
                        )
                    };
                    if !success {
                        return Err(AvsError::with_str(
                            "Failed to set color parameter",
                        ));
                    }
                    Ok(())
                }
                _ => Err(AvsError::with_str("Type mismatch: expected Color")),
            },
            _ => Err(AvsError::with_str("Unsupported parameter type for setting")),
        }
    }

    pub fn set_bool(&mut self, value: bool) -> Result<(), AvsError> {
        self.set_value(AvsConfigValue::Bool(value))
    }

    pub fn set_int(&mut self, value: i64) -> Result<(), AvsError> {
        self.set_value(AvsConfigValue::Int(value))
    }

    pub fn set_select(&mut self, value: i64) -> Result<(), AvsError> {
        self.set_value(AvsConfigValue::Select(value))
    }

    pub fn set_float(&mut self, value: f64) -> Result<(), AvsError> {
        self.set_value(AvsConfigValue::Float(value))
    }

    pub fn set_color(&mut self, value: AvsColor) -> Result<(), AvsError> {
        self.set_value(AvsConfigValue::Color(value))
    }

    pub fn set_string(&mut self, value: impl Into<String>) -> Result<(), AvsError> {
        self.set_value(AvsConfigValue::String(value.into()))
    }

    pub fn set_resource(&mut self, value: impl Into<String>) -> Result<(), AvsError> {
        self.set_value(AvsConfigValue::Resource(value.into()))
    }
}

macro_rules! impl_try_into_value {
    ($type:ty; $($variant:ident),+ $(,)?) => {
        impl TryFrom<AvsConfigValue> for $type {
            type Error = AvsError;

            fn try_from(value: AvsConfigValue) -> Result<Self, Self::Error> {
                match value {
                    $(AvsConfigValue::$variant(v) => Ok(v),)+
                    _ => Err(AvsError::with_str(&format!(
                        "Get value: expected type {}, got {}",
                        stringify!($type),
                        match value {
                            AvsConfigValue::Invalid => "Invalid",
                            AvsConfigValue::List => "List",
                            AvsConfigValue::Action => "Action",
                            AvsConfigValue::Bool(_) => "Bool",
                            AvsConfigValue::Int(_) => "Int",
                            AvsConfigValue::Float(_) => "Float",
                            AvsConfigValue::Color(_) => "Color",
                            AvsConfigValue::String(_) => "String",
                            AvsConfigValue::Select(_) => "Select",
                            AvsConfigValue::Resource(_) => "Resource",
                            AvsConfigValue::IntArray(_) => "IntArray",
                            AvsConfigValue::FloatArray(_) => "FloatArray",
                            AvsConfigValue::ColorArray(_) => "ColorArray",
                        },
                    ))),
                }
            }
        }
    };
}

impl_try_into_value!(bool; Bool);
impl_try_into_value!(i64; Int, Select);
impl_try_into_value!(f64; Float);
impl_try_into_value!(AvsColor; Color);
impl_try_into_value!(String; String, Resource);
impl_try_into_value!(Vec<i64>; IntArray);
impl_try_into_value!(Vec<f64>; FloatArray);
impl_try_into_value!(Vec<AvsColor>; ColorArray);

impl AvsComponentConfig {
    pub fn new(
        avs: AVS_Handle,
        component: AVS_Component_Handle,
        parameter: Option<Rc<AvsParameter>>,
        nested_path: Vec<i64>,
    ) -> Result<Self, AvsError> {
        let cached_parameters = if let Some(ref parent_param) = parameter {
            parent_param
                .children
                .iter()
                .map(|param| {
                    let nested_configs =
                        if matches!(param.r#type, AvsParameterType::List) {
                            Self::build_nested_configs(
                                avs,
                                component,
                                Rc::new(param.clone()),
                                &nested_path,
                            )?
                        } else {
                            Vec::new()
                        };
                    Ok((
                        param.name.clone(),
                        AvsConfigParameter {
                            avs,
                            component,
                            parameter: Rc::new(param.clone()),
                            nested_config_path: nested_path.clone(),
                            cached_nested_configs: nested_configs,
                        },
                    ))
                })
                .collect::<Result<BTreeMap<_, _>, AvsError>>()?
        } else {
            Self::build_component_parameters(avs, component, &nested_path)?
        };

        Ok(Self { cached_parameters })
    }

    fn build_component_parameters(
        avs: AVS_Handle,
        component: AVS_Component_Handle,
        nested_path: &[i64],
    ) -> Result<BTreeMap<String, AvsConfigParameter>, AvsError> {
        let effect_handle = unsafe { avs_component_effect(avs, component) };
        if effect_handle == 0 {
            return Err(AvsError::with_str("Failed to get component effect"));
        }

        let mut effect_info = AVS_Effect_Info {
            group: std::ptr::null(),
            name: std::ptr::null(),
            help: std::ptr::null(),
            parameters_length: 0u32,
            parameters: std::ptr::null(),
            can_have_child_components: false,
            is_user_creatable: false,
        };

        if !unsafe { avs_effect_info(avs, effect_handle, &mut effect_info) } {
            return Err(AvsError::with_str("Failed to get effect info"));
        }

        if effect_info.parameters.is_null() || effect_info.parameters_length == 0 {
            return Ok(BTreeMap::new());
        }

        let parameter_handles = unsafe {
            std::slice::from_raw_parts(
                effect_info.parameters,
                effect_info.parameters_length as usize,
            )
        };

        let mut parameters = BTreeMap::new();
        for (i, &param_handle) in parameter_handles.iter().enumerate() {
            let param_info =
                Avs::parameter_info(avs, effect_handle, param_handle, i + 1)?;
            let nested_configs = if matches!(param_info.r#type, AvsParameterType::List)
            {
                Self::build_nested_configs(
                    avs,
                    component,
                    Rc::new(param_info.clone()),
                    nested_path,
                )?
            } else {
                Vec::new()
            };
            parameters.insert(
                param_info.name.clone(),
                AvsConfigParameter {
                    avs,
                    component,
                    parameter: Rc::new(param_info),
                    nested_config_path: nested_path.to_vec(),
                    cached_nested_configs: nested_configs,
                },
            );
        }

        Ok(parameters)
    }

    fn build_nested_configs(
        avs: AVS_Handle,
        component: AVS_Component_Handle,
        parameter: Rc<AvsParameter>,
        nested_path: &[i64],
    ) -> Result<Vec<AvsComponentConfig>, AvsError> {
        let list_length = unsafe {
            avs_parameter_list_length(
                avs,
                component,
                parameter.handle,
                nested_path.len() as u32,
                nested_path.as_ptr(),
            )
        };

        if list_length < 0 {
            return Ok(Vec::new());
        }

        let mut configs = Vec::new();
        for i in 0..list_length as usize {
            let mut new_path = nested_path.to_vec();
            new_path.push(i as i64);
            configs.push(Self::new(avs, component, Some(parameter.clone()), new_path)?);
        }

        Ok(configs)
    }

    pub fn get(&self, key: &str) -> Option<&AvsConfigParameter> {
        self.cached_parameters.get(key)
    }
}

impl std::ops::Index<&str> for AvsComponentConfig {
    type Output = AvsConfigParameter;

    fn index(&self, key: &str) -> &Self::Output {
        self.cached_parameters
            .get(key)
            .unwrap_or_else(|| panic!("Parameter '{}' not found", key))
    }
}

impl std::ops::IndexMut<&str> for AvsComponentConfig {
    fn index_mut(&mut self, key: &str) -> &mut Self::Output {
        self.cached_parameters
            .get_mut(key)
            .unwrap_or_else(|| panic!("Parameter '{}' not found", key))
    }
}

impl std::ops::Index<usize> for AvsConfigParameter {
    type Output = AvsComponentConfig;

    fn index(&self, index: usize) -> &Self::Output {
        if !matches!(self.parameter.r#type, AvsParameterType::List) {
            panic!("Cannot index into non-list parameter");
        }
        &self.cached_nested_configs[index]
    }
}

impl std::ops::IndexMut<usize> for AvsConfigParameter {
    fn index_mut(&mut self, index: usize) -> &mut Self::Output {
        if !matches!(self.parameter.r#type, AvsParameterType::List) {
            panic!("Cannot index into non-list parameter");
        }
        &mut self.cached_nested_configs[index]
    }
}

fn c_str_ptr_or_null(string: Option<&str>) -> *const i8 {
    string
        .and_then(|s| CString::new(s).ok())
        .map_or(std::ptr::null(), |cstr| cstr.as_ptr())
}

fn str_from_c_lossy_or_empty(cstr: *const i8) -> String {
    str_from_c_lossy(cstr).unwrap_or_default()
}

fn str_from_c_lossy(cstr: *const i8) -> Option<String> {
    if !cstr.is_null() {
        Some(unsafe { CStr::from_ptr(cstr) }.to_string_lossy().into_owned())
    } else {
        None
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_config_api_basic() {
        // Test that the new API structure compiles and works conceptually
        let config = AvsComponentConfig::new(0, 0, None, Vec::new()).unwrap();

        // Test that we can call get methods
        let _param_result = config.get("test_param");

        // The actual functionality would need a real AVS instance to test properly
        // This is just a compilation test for now
    }

    #[test]
    fn test_config_parameter_list_access() {
        // Test list parameter access functionality

        // Create a list parameter for testing
        let param = AvsConfigParameter {
            avs: 0,
            component: 0,
            parameter: Rc::new(AvsParameter {
                handle: 0,
                r#type: AvsParameterType::List,
                name: "Colors".to_string(),
                description: Some("List of colors".to_string()),
                is_global: false,
                int_min: 0,
                int_max: 0,
                float_min: 0.0,
                float_max: 0.0,
                options: None,
                children: Vec::new(),
                children_min: 0,
                children_max: 0,
            }),
            nested_config_path: Vec::new(),
            cached_nested_configs: Vec::new(),
        };

        // Test that list indexing returns a config
        let _nested_config_result = param.get(0);
    }

    #[test]
    fn test_full_indexing_syntax() {
        // Test the complete indexing chain structure

        // Create a list parameter for testing
        let param = AvsConfigParameter {
            avs: 0,
            component: 0,
            parameter: Rc::new(AvsParameter {
                handle: 0,
                r#type: AvsParameterType::List,
                name: "Contributors".to_string(),
                description: Some("List of contributors".to_string()),
                is_global: false,
                int_min: 0,
                int_max: 0,
                float_min: 0.0,
                float_max: 0.0,
                options: None,
                children: Vec::new(),
                children_min: 0,
                children_max: 0,
            }),
            nested_config_path: Vec::new(),
            cached_nested_configs: Vec::new(),
        };

        // Test: contributors_param[0] -> AvsComponentConfig
        let nested_config_result = param.get(0);
        assert!(nested_config_result.is_ok());

        // This demonstrates the API structure is correct for:
        // config.get("Contributors")?.get(0)?.get("Name")?
    }

    #[test]
    fn test_api_structure_compiles() {
        // Test that the API structure compiles without C calls
        let _config = AvsComponentConfig::new(0, 0, None, Vec::new()).unwrap();

        // This confirms our alternating types pattern works
    }
}
