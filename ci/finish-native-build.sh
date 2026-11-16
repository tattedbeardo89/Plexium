#!/usr/bin/env bash
# Resume only after the remaining source host is permitted. Does not deploy.
set -euo pipefail
cd "$(dirname "$0")/.."
export PS5_VULKAN=/workspace/deps/PS5_Vulkan
export PS5_PAYLOAD_SDK=$PS5_VULKAN/.deps/native/ps5-payload-sdk
export VLC_PS5_WORK=/workspace/plexium-work
export BUILD_JOBS=2 WGETRC=/tmp/plexium-wgetrc
export PATH="$VLC_PS5_WORK/tools/bin:$PATH"
python3 tools/fetch-native-sources.py "$VLC_PS5_WORK/dl"
TARGET=ps5 bash ps5/deps/build-deps.sh freetype harfbuzz ass bluray
TARGET=ps5 bash ps5/configure-vlc.sh
bash ci/build-native.sh
