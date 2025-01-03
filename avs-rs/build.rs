// use cmake;
// use std::env;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let bindings = bindgen::Builder::default()
        .allowlist_function("^avs_.*")
        .allowlist_type("^AVS_.*")
        .constified_enum("^AVS_.*")
        .prepend_enum_name(false)
        .translate_enum_integer_types(true)
        .formatter(bindgen::Formatter::Prettyplease)
        .header("../avs/vis_avs/avs.h")
        .header("../avs/vis_avs/avs_editor.h")
        .generate()?;
    bindings.write_to_file("src/bindings.rs")?;

    // if cfg!(target_os = "linux") && cfg!(target_arch = "x86") {
    //     env::set_var("PKG_CONFIG_PATH", "/usr/lib32/pkgconfig");
    // }
    // cmake::Config::new("..").out_dir("..").build_target("libavs").build();

    // println!("cargo::rustc-link-search=native=../build");
    println!("cargo::rustc-link-search=../build");
    println!("cargo::rustc-env=LD_LIBRARY_PATH=../build");
    // println!("cargo::rerun-if-changed=avs/vis_avs/avs.h");

    Ok(())
}
