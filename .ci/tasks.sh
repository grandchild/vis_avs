#!/usr/bin/env bash

shopt -s globstar # Enable ** globbing

# If the command run by git-bisect exits with 125 the commit is skipped and considered
# unknown whether good or bad; e.g. failing to set up the build does not technically
# mean the build failed.
GIT_BISECT_CANNOT_CHECK=125

function clean() {
    rm -rf build_win32
    rm -rf build_linux32
    rm -rf target
}

function check_clang_format() {
    dump=${1:-}
    if [[ $dump == "dump" ]]; then
        echo .clang-format
        clang-format -style=file -dump-config
        echo
    fi
    srcdir=$(pwd)
    tmpclone=$(mktemp -d)
    git clone -q "$srcdir" "$tmpclone" || return $GIT_BISECT_CANNOT_CHECK
    cd "$tmpclone" || return $GIT_BISECT_CANNOT_CHECK
    git ls-files '*.c' '*.cpp' '*.h' \
        | grep -vf <(grep -rl "^DisableFormat: *true" "$tmpclone"/**/.clang-format \
            | sed "s:/.clang-format::;s:$tmpclone/::") \
        | xargs clang-format -i -style=file
    git diff --exit-code || return 1
    rm -rf "$tmpclone"
}

function build_win32() {
    build_dir=build_win32
    mkdir -p "$build_dir"
    pushd "$build_dir" || return 1
    if grep -q "Arch Linux" /etc/os-release; then
        i686-w64-mingw32-cmake .. || return $GIT_BISECT_CANNOT_CHECK
    else
        cmake -DCMAKE_TOOLCHAIN_FILE=CMake-MingWcross-toolchain.txt .. \
            || return $GIT_BISECT_CANNOT_CHECK
    fi
    make -k -j "$parallel" all
    popd || return 1
}

function run_winamp() {
    if [ ! -e build_win32/vis_avs.dll ]; then
        build_win32 || return
    fi
    winamp_dir=$1
    cp build_win32/vis_avs.dll "$winamp_dir/Plugins/"
    export WINEARCH=win32
    wine "$winamp_dir/winamp.exe"
}

function build_linux32() {
    build_dir=build_linux32
    cmake -B "$build_dir" --toolchain CMake-Linux32cross-toolchain.txt || return 1
    cmake --build "$build_dir" --parallel "$parallel"
}

function run_c_cli() {
    if [ ! -e build_linux32/libavs.so ]; then
        build_linux32 || return
    fi
    build_dir=build_linux32
    export LD_LIBRARY_PATH=$build_dir
    "$build_dir/avs-cli" "$1"
}

function run_rust_cli() {
    if [ ! -e build_linux32/libavs.so ]; then
        build_linux32 || return
    fi
    build_dir=build_linux32
    export RUST_BACKTRACE=${RUST_BACKTRACE:-1}
    export RUSTFLAGS="-L $build_dir -l avs"
    export LD_LIBRARY_PATH=$build_dir
    export PKG_CONFIG_SYSROOT_DIR=/usr/lib32/
    cargo run \
        --bin avs-cli \
        --target i686-unknown-linux-gnu \
        -- \
        "$1"
}

task=$1
shift

parallel=${JOBS:-$(nproc --ignore=1)}
case $task in
    "build-win32")
        (build_win32)
        ;;
    "run-winamp")
        (run_winamp "$1")
        ;;
    "build-linux32")
        (build_linux32)
        ;;
    "run-c-cli")
        (run_c_cli "$1")
        ;;
    "run-rust-cli")
        (run_rust_cli "$1")
        ;;
    "check-clang-format")
        (check_clang_format "$1")
        ;;
    "all-checks")
        (echo -n "clang-format " ; check_clang_format &> /dev/null && echo ✓ || echo ✗) \
            && (echo -n "build_win32 " ; build_win32 &> /dev/null && echo ✓ || echo ✗) \
            && (echo -n "build_linux32 " ; build_linux32 &> /dev/null && echo ✓ || echo ✗)
        ;;
    "clean")
        (clean)
        ;;
    *)
        echo "Usage: $0 TASK"
        echo
        echo "Tasks:"
        echo "    build-win32"
        echo "    run-winamp <winamp-dir>"
        echo "    build-linux32"
        echo "    run-rust-cli <preset>"
        echo "    check-clang-format [dump]"
        echo "    all-checks"
        echo "    clean"
        echo
        echo "Env vars:"
        echo "    JOBS - Number of parallel jobs to run."
        echo "           Defaults to nproc - 1"
        echo "    WINEPREFIX - Wine prefix to use for Winamp."
        echo "                 Defaults to ~/.wine and will fail if that's 64bit!"
        ;;
esac || exit
