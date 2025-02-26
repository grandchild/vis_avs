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
    verbose=${1:-}
    if [ "$verbose" == "verbose" ]; then
        _verbose="VERBOSE=1"
    fi
    build_dir=build_win32
    mkdir -p "$build_dir"
    pushd "$build_dir" || return 1
    if grep -q "Arch Linux" /etc/os-release; then
        i686-w64-mingw32-cmake .. || return $GIT_BISECT_CANNOT_CHECK
    else
        cmake -DCMAKE_TOOLCHAIN_FILE=CMake-MingWcross-toolchain.txt .. \
            || return $GIT_BISECT_CANNOT_CHECK
    fi
    # shellcheck disable=SC2086  # $_verbose should be omitted if empty
    make -k -j "$parallel" $_verbose all
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
    verbose=${1:-}
    if [ "$verbose" == "verbose" ]; then
        _verbose="VERBOSE=1"
    fi
    build_dir=build_linux32
    mkdir -p "$build_dir"
    pushd "$build_dir" || return 1
    cmake -DCMAKE_TOOLCHAIN_FILE=CMake-Linux32cross-toolchain.txt .. \
        || return $GIT_BISECT_CANNOT_CHECK
    # shellcheck disable=SC2086  # $_verbose should be omitted if empty
    make -k -j "$parallel" $_verbose
    popd || return 1
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

function install_deps() {
    distro=${1:-}
    if [[ $distro == "arch" || $distro == "archlinux" ]]; then
        if ! command -v yay &> /dev/null; then
            echo "Error: 'yay' is used for Arch Linux dependency installation."
            echo "Edit the script to use another AUR helper if desired."
            exit 1
        fi
        yay -Sy --noconfirm --needed \
            base-devel \
            mingw-w64-cmake \
            mingw-w64-gcc \
            mingw-w64-ffmpeg-minimal \
            lib32-util-linux \
            lib32-libpipewire \
            lib32-ffmpeg-minimal-dev \
            #
    elif [[ $distro == "ubuntu" || $distro == "debian" ]]; then
        SUDO=sudo
        if ! command $SUDO -v &> /dev/null; then
            SUDO=""
        fi
        $SUDO apt-get update
        $SUDO apt-get install -y --no-install-recommends \
            cmake pkg-config mingw-w64 gcc-multilib g++-multilib \
            autoconf gettext flex bison libtool autopoint \
            libavformat-dev libavcodec-dev libswscale-dev \
            libpipewire-0.3-dev \
            #
    fi
}

function build_libuuid_32bit() {
    install=${1:-}
    tmpclone=$(mktemp -d)
    git clone \
        https://git.kernel.org/pub/scm/utils/util-linux/util-linux.git \
        "$tmpclone" \
        --depth=1
    pushd "$tmpclone" || return 1
    # requires: autoconf gettext flex bison libtool autopoint
    ./autogen.sh
    ./configure \
        --host=i686-linux-gnu \
        "CFLAGS=-m32" "CXXFLAGS=-m32" "LDFLAGS=-m32" \
        --libdir=/usr/lib32/ \
        --disable-all-programs \
        --enable-libuuid
    make -j "$parallel"
    if [[ "$install" == "install" ]]; then
        sudo make install
    fi
}

task=$1
shift

parallel=${JOBS:-$(nproc --ignore=1)}
case $task in
    "build-win32")
        (build_win32 "$1")
        ;;
    "run-winamp")
        (run_winamp "$1")
        ;;
    "build-linux32")
        (build_linux32 "$1")
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
            && (echo -n "build_win32 " ; build_win32 "$1" &> /dev/null && echo ✓ || echo ✗) \
            && (echo -n "build_linux32 " ; build_linux32 "$1" &> /dev/null && echo ✓ || echo ✗)
        ;;
    "clean")
        (clean)
        ;;
    "install-deps")
        (install_deps "$1")
        ;;
    "build-libuuid-32bit")
        (build_libuuid_32bit "$1")
        ;;
    *)
        echo "Usage: $0 TASK"
        echo
        echo "Tasks:"
        echo "    build-win32 [verbose]"
        echo "    run-winamp <winamp-dir>"
        echo "    build-linux32 [verbose]"
        echo "    run-rust-cli <preset>"
        echo "    check-clang-format [dump]"
        echo "    all-checks [verbose]"
        echo "    clean"
        echo "    install-deps [arch|ubuntu|debian]"
        echo "    build-libuuid-32bit [install] (only needed on debian/ubuntu)"
        echo "        run 'install-deps' or install"
        echo "            autoconf autopoint gettext flex bison libtool"
        echo "        before running this task"
        echo
        echo "Env vars:"
        echo "    JOBS - Number of parallel jobs to run."
        echo "           Defaults to nproc - 1"
        echo "    WINEPREFIX - Wine prefix to use for Winamp."
        echo "                 Defaults to ~/.wine and will fail if that's 64bit!"
        exit 1
        ;;
esac || exit
