use std::env;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=../avs/vis_avs/avs.h");
    println!("cargo:rerun-if-changed=../avs/vis_avs/avs_editor.h");
    bindgen::Builder::default()
        .allowlist_function("^avs_.*")
        .allowlist_type("^AVS_.*")
        .constified_enum("^AVS_.*")
        .prepend_enum_name(false)
        .translate_enum_integer_types(true)
        .formatter(bindgen::Formatter::Prettyplease)
        .header("../avs/vis_avs/avs.h")
        .header("../avs/vis_avs/avs_editor.h")
        .generate()?
        .write_to_file("bindings/bindings.rs")?;

    if let Ok(lib_dir) = env::var("AVS_LIB_DIR") {
        println!("cargo:rustc-link-search=native={lib_dir}");
    }
    println!("cargo:rustc-link-lib=dylib=avs");

    Ok(())
}
