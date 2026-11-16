#!/usr/bin/env bash
set -euo pipefail
[[ -r /run/ca-bundle.pem ]] || { echo 'Mount a combined system/proxy CA bundle read-only at /run/ca-bundle.pem'; exit 2; }
printf 'Server = https://archive.archlinux.org/repos/2026/10/20/$repo/os/$arch\n' > /etc/pacman.d/mirrorlist
sed -i 's|^#XferCommand = /usr/bin/curl.*|XferCommand = /usr/bin/curl --fail --location --cacert /run/ca-bundle.pem --output %o %u|' /etc/pacman.conf
pacman-key --init
pacman-key --populate archlinux
pacman -Syyuu --noconfirm --needed base-devel clang llvm lld llvm-libs libclc spirv-llvm-translator spirv-tools glslang meson ninja cmake python python-mako python-markupsafe python-ply python-packaging python-yaml git curl wget unzip zip tar pkgconf rsync nasm ccache autoconf automake libtool gettext gperf bison flex texinfo perl help2man ca-certificates
pacman -Q > /workspace/Plexium/artifacts/packages.txt
sed -i '/^XferCommand = /d' /etc/pacman.conf
printf 'ca_certificate = /run/ca-bundle.pem\n' > /tmp/plexium-wgetrc
