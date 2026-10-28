#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
headers=${VULKAN_HEADERS:-/workspace/deps/Vulkan-Headers/include}
[[ -f $headers/vulkan/vulkan.h ]] || { echo 'VULKAN_HEADERS must point to pinned Vulkan headers' >&2; exit 2; }
g++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation -fno-exceptions -fno-rtti -DVK_NO_PROTOTYPES \
 -Isrc -Ithird_party -Ithird_party/imgui -Ithird_party/stb -Ithird_party/volk -I"$headers" \
 -fsyntax-only src/main.cc src/textures.cc src/app/session.cc src/app/view.cc src/plex/client.cc src/plex/transport_ps5.cc src/plex/artwork.cc
echo 'PASS: native entry, shared application, protocol, PS5 transport and artwork host syntax (not a cross-build)'
