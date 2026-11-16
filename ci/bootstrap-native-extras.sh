#!/usr/bin/env bash
set -euo pipefail
export PS5_VULKAN=/workspace/deps/PS5_Vulkan
export PS5_PAYLOAD_SDK=$PS5_VULKAN/.deps/native/ps5-payload-sdk
export VLC_PS5_WORK=/workspace/plexium-work
export BUILD_JOBS=2 WGETRC=/tmp/plexium-wgetrc
export PATH="$VLC_PS5_WORK/tools/bin:$PATH"
TARGET=ps5 bash ps5/deps/build-deps.sh libarchive iconv fribidi
