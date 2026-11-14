#!/usr/bin/env bash
set -euo pipefail
export PS5_VULKAN=/workspace/deps/PS5_Vulkan
export PS5_PAYLOAD_SDK=$PS5_VULKAN/.deps/native/ps5-payload-sdk
export PS5_PAYLOAD_SDK_FORK=/workspace/deps/PS5_PayloadSDK
export PS5_MESA_FORK=/workspace/deps/PS5_Mesa
export PS5_MESA_REVISION=1ba9874f478657ccf6da72ce55f0260164cc0bd0
export WGETRC=/tmp/plexium-wgetrc BUILD_JOBS=2
mkdir -p /workspace/plexium-work/tools/bin
printf '#!/usr/bin/env bash\nexec /usr/bin/ninja -j2 "$@"\n' > /workspace/plexium-work/tools/bin/ninja
chmod +x /workspace/plexium-work/tools/bin/ninja
export PATH=/workspace/plexium-work/tools/bin:$PATH
export NINJA=/workspace/plexium-work/tools/bin/ninja
bash "$PS5_VULKAN/tools/rebuild-libc.sh"
zlib=$(find "$PS5_VULKAN/.deps/native/zlib/root" -name libz.a -type f -print -quit)
mkdir -p "$PS5_VULKAN/build/host"
clang++ -std=c++20 -O2 -Wall -Wextra -Werror -I"$PS5_VULKAN/.deps/native/zlib/root/usr/include" \
 "$PS5_VULKAN/tooling/native/native_app_builder.cpp" "$PS5_VULKAN/tooling/native/self_container.cpp" \
 "$PS5_VULKAN/tooling/native/elf_object.cpp" "$PS5_VULKAN/tooling/native/sce_module_writer.cpp" "$zlib" -o "$PS5_VULKAN/build/host/ps5-native-tool"
# Supported offline package cache; preserve the wrap's original expected hashes.
cache=$PS5_VULKAN/.deps/work/radv-src/subprojects/packagecache
mkdir -p "$cache"
if [[ ! -f $cache/zlib_1.3.1-1_patch.zip ]]; then
 curl --fail --location --proto '=https' --proto-redir '=https' https://github.com/mesonbuild/wrapdb/releases/download/zlib_1.3.1-1/zlib_1.3.1-1_patch.zip -o "$cache/zlib_1.3.1-1_patch.zip"
fi
printf '%s  %s\n' e79b98eb24a75392009cec6f99ca5cdca9881ff20bfa174e8b8926d5c7a47095 "$cache/zlib_1.3.1-1_patch.zip" | sha256sum --check
bash "$PS5_VULKAN/tools/build-radv.sh" release
PS5_VULKAN="$PS5_VULKAN" bash ps5/radv-patches.sh
