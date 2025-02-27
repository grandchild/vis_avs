FROM archlinux:multilib-devel

RUN useradd -m build -s /bin/bash
RUN echo "build ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
# disable makepkg debug builds
RUN sed -i 's/ debug/ !debug/' /etc/makepkg.conf
USER build
WORKDIR /home/build

RUN sudo pacman -Syu --needed --noconfirm \
        git \
        mingw-w64-gcc \
        clang \
        rustup \
        go \
        wine \
    && rustup set profile minimal \
    && rustup default stable \
    && rustup target add i686-pc-windows-gnu \
    \
    && git config --global init.defaultBranch main \
    && git clone https://aur.archlinux.org/paru.git \
    && sh -c "cd paru && makepkg -fsri --noconfirm" \
    \
    && git clone --branch=ci-updates https://github.com/grandchild/vis_avs.git \
    && vis_avs/.ci/tasks.sh install-deps archlinux \
    && sudo pacman -Scc \
    && rustup toolchain uninstall stable-x86_64-unknown-linux-gnu \
    && rm -rf paru .cache .cargo .wine
