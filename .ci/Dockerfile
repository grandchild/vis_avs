FROM archlinux:multilib-devel AS buildimg

RUN sed -i 's/ debug/ !debug/' /etc/makepkg.conf

RUN useradd -m build -s /bin/bash
RUN echo "build ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
USER build
WORKDIR /home/build

# Install basic dependencies for installing AUR packages:
# - git: for cloning yay repo & AUR packages
# - rustup: Rust toolchain for AVS frontends
# - yay: AUR helper
# - Install dependencies for vis_avs
# - Clean up to reduce image size a bit
RUN sudo pacman -Syu --needed --noconfirm git rustup

RUN rustup set profile minimal
RUN rustup default stable
RUN rustup target add i686-unknown-linux-gnu

RUN git config --global init.defaultBranch main
RUN git clone https://aur.archlinux.org/yay.git
RUN sh -c "cd yay && makepkg -fsri --noconfirm"
COPY . /ci
RUN /ci/tasks.sh install-deps archlinux

RUN sudo pacman -Scc --noconfirm
RUN rustup toolchain uninstall stable-x86_64-unknown-linux-gnu
RUN rm -rf yay .cache .cargo .wine

FROM scratch AS builder
COPY --from=buildimg / /
