#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
headers=${VULKAN_HEADERS:-/workspace/deps/Vulkan-Headers/include}
out=build
flags=()
if [[ ${SANITIZE:-0} == 1 ]]; then out=build/san; flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
mkdir -p "$out"
g++ -std=c++17 -O1 -g -ffunction-sections -fdata-sections -DVK_NO_PROTOTYPES \
 -Isrc -Ithird_party/volk -I"$headers" "${flags[@]}" tests/gfx_failure_tests.cc \
 -Wl,--gc-sections -ldl -o "$out/gfx-failure-tests"
"$out/gfx-failure-tests"
obj=build/obj
[[ ${SANITIZE:-0} != 1 ]] || obj=build/obj-san
make -j"${BUILD_JOBS:-2}" SANITIZE="${SANITIZE:-0}" "$obj"/third_party/imgui/{imgui,imgui_draw,imgui_tables,imgui_widgets}.o >/dev/null
g++ -std=c++17 -O1 -g -ffunction-sections -fdata-sections -DVK_NO_PROTOTYPES \
 -Isrc -Ithird_party/imgui -Ithird_party/volk -I"$headers" "${flags[@]}" tests/ui_backend_failure_tests.cc \
 "$obj"/third_party/imgui/{imgui,imgui_draw,imgui_tables,imgui_widgets}.o \
 -Wl,--gc-sections -ldl -o "$out/ui-backend-failure-tests"
"$out/ui-backend-failure-tests"
g++ -std=c++17 -O1 -g -ffunction-sections -fdata-sections -DVK_NO_PROTOTYPES \
 -Isrc -Ithird_party/volk -I"$headers" "${flags[@]}" tests/gfx_ownership_tests.cc \
 -Wl,--gc-sections -ldl -o "$out/gfx-ownership-tests"
"$out/gfx-ownership-tests"

g++ -std=c++17 -O1 -g -ffunction-sections -fdata-sections -DVK_NO_PROTOTYPES \
 -Isrc -Ithird_party/imgui -Ithird_party/stb -Ithird_party/volk -I"$headers" "${flags[@]}" tests/texture_lifecycle_tests.cc third_party/volk/volk.c \
 -Wl,--gc-sections -ldl -o "$out/texture-lifecycle-tests"
"$out/texture-lifecycle-tests"
