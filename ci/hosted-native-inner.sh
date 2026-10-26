#!/usr/bin/env bash
set -euo pipefail
cd /workspace/Plexium
export PS5_VULKAN=/workspace/deps/PS5_Vulkan PS5_PAYLOAD_SDK_FORK=/workspace/deps/PS5_PayloadSDK
export PS5_MESA_FORK=/workspace/deps/PS5_Mesa VLC_PS5_WORK=/workspace/plexium-work
export BUILD_JOBS=2 WGETRC=/tmp/plexium-wgetrc
export PS5_PAYLOAD_SDK=$PS5_VULKAN/.deps/native/ps5-payload-sdk
git config --global --add safe.directory /workspace/Plexium
for name in PS5_Vulkan PS5_PayloadSDK PS5_Mesa; do
 pin=$(python3 -c 'import json,sys;print(json.load(open("ci/build-lock.json"))[sys.argv[1]])' "$name")
 root=/workspace/deps/$name
 git config --global --add safe.directory "$root"
 if [[ ! -d $root/.git ]]; then
  mkdir -p "$root"
  git -C "$root" init -q
  git -C "$root" fetch --depth=1 "https://github.com/mihawk-99/$name.git" "$pin"
  git -C "$root" checkout --detach "$pin"
 fi
 [[ $(git -C "$root" rev-parse HEAD) == "$pin" ]] || { echo "Wrong cached revision: $name"; exit 2; }
 git -C "$root" diff --exit-code --quiet
 # Source kit creation reads origins only as source metadata, never credentials.
done
fingerprint=$(find ci/build-lock.json ci/source-checksums.txt ps5 patches -type f -print0 | sort -z | xargs -0 sha256sum | sha256sum | cut -d' ' -f1)
if [[ ! -f $VLC_PS5_WORK/.ci-dependencies || $(cat "$VLC_PS5_WORK/.ci-dependencies") != "$fingerprint" ]]; then
 bash ci/bootstrap-sdk.sh
 bash ci/bootstrap-native-runtime.sh
 python3 tools/fetch-native-sources.py "$VLC_PS5_WORK/dl"
 archive=$VLC_PS5_WORK/dl/vlc-3.0.24.tar.xz
 if [[ ! -f $archive ]]; then
  curl --fail --location --proto '=https' --proto-redir '=https' https://download.videolan.org/pub/videolan/vlc/3.0.24/vlc-3.0.24.tar.xz -o "$archive"
 fi
 printf '%s  %s\n' e7cab503d1d7d5849b89d2cf0e1ee60d0ef6d012407791b644b9cfc0cc225fdf "$archive" | sha256sum --check
 mkdir -p "$VLC_PS5_WORK/src"
 [[ -d $VLC_PS5_WORK/src/vlc-3.0.24 ]] || tar -xJf "$archive" -C "$VLC_PS5_WORK/src"
 export PATH="$VLC_PS5_WORK/tools/bin:$PATH"
 TARGET=ps5 bash ps5/deps/build-deps.sh
 TARGET=ps5 bash ps5/configure-vlc.sh
 printf '%s\n' "$fingerprint" > "$VLC_PS5_WORK/.ci-dependencies"
fi
export PATH="$VLC_PS5_WORK/tools/bin:$PATH"
bash ci/build-native.sh
python3 ci/record-native-build.py
git diff --exit-code -- src assets ps5/sce_sys
python3 tools/make-source-kit.py
(cd artifacts && sha256sum Plexium-PPSA85421.zip Plexium-native-debug.elf Plexium-native-source.tar.gz Plexium-source.bundle > SHA256SUMS)
cp docs/INSTALLATION.md docs/DELIVERY.md artifacts/
