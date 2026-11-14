#!/usr/bin/env bash
set -euo pipefail
export PS5_VULKAN=/workspace/deps/PS5_Vulkan
export PS5_PAYLOAD_SDK_FORK=/workspace/deps/PS5_PayloadSDK
export PS5_MESA_FORK=/workspace/deps/PS5_Mesa
export PS5_MESA_REVISION=1ba9874f478657ccf6da72ce55f0260164cc0bd0
export WGETRC=/tmp/plexium-wgetrc
export BUILD_JOBS=2
for path in "$PS5_VULKAN" "$PS5_PAYLOAD_SDK_FORK" "$PS5_MESA_FORK"; do git config --global --add safe.directory "$path"; done
bash "$PS5_VULKAN/tools/setup-native-dependencies.sh"
