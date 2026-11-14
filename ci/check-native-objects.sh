#!/usr/bin/env bash
# Real PS5-target objects; not a linked/signed executable or hardware result.
set -euo pipefail
cd "$(dirname "$0")/.."
vk=${PS5_VULKAN:-/workspace/deps/PS5_Vulkan}
export PS5_PAYLOAD_SDK=${PS5_PAYLOAD_SDK:-$vk/.deps/native/ps5-payload-sdk}
export PS5_CLANG=${PS5_CLANG:-/usr/bin/clang}
headers=${VULKAN_HEADERS:-/workspace/deps/Vulkan-Headers/include}
mkdir -p build/native-check
for source in src/main.cc src/app/session.cc src/app/view.cc src/textures.cc src/plex/client.cc src/plex/transport_ps5.cc src/plex/artwork.cc; do
 PS5_PAYLOAD_SDK="$PS5_PAYLOAD_SDK" sh "$vk/tooling/prospero-clang18" -std=c++17 -O2 -g -fno-exceptions -fno-rtti \
  -march=znver2 -DPLEX_PS5 -DVK_NO_PROTOTYPES -DIMGUI_IMPL_VULKAN_USE_VOLK \
  -I/workspace/plexium-work/prefix-ps5/include -Isrc -Ithird_party -Ithird_party/imgui -Ithird_party/stb -Ithird_party/volk -I"$headers" \
  -c "$source" -o "build/native-check/$(basename "${source%.cc}").o"
done
file build/native-check/*.o
printf 'PASS: seven real PS5-target application objects; link/package/hardware remain unverified\n'
