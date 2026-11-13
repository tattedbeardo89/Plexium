#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
headers=${VULKAN_HEADERS:-/workspace/deps/Vulkan-Headers/include}
mkdir -p build
flags=()
if [[ ${SANITIZE:-0} == 1 ]]; then flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
g++ -std=c++17 -O1 -g -ffunction-sections -fdata-sections -DVK_NO_PROTOTYPES \
 -Isrc -Ithird_party/volk -I"$headers" "${flags[@]}" tests/gfx_failure_tests.cc \
 -Wl,--gc-sections -ldl -o build/gfx-failure-tests
build/gfx-failure-tests
obj=build/obj
[[ ${SANITIZE:-0} != 1 ]] || obj=build/obj-san
make -j"${BUILD_JOBS:-2}" SANITIZE="${SANITIZE:-0}" "$obj"/third_party/imgui/{imgui,imgui_draw,imgui_tables,imgui_widgets}.o >/dev/null
g++ -std=c++17 -O1 -g -ffunction-sections -fdata-sections -DVK_NO_PROTOTYPES \
 -Isrc -Ithird_party/imgui -Ithird_party/volk -I"$headers" "${flags[@]}" tests/ui_backend_failure_tests.cc \
 "$obj"/third_party/imgui/{imgui,imgui_draw,imgui_tables,imgui_widgets}.o \
 -Wl,--gc-sections -ldl -o build/ui-backend-failure-tests
build/ui-backend-failure-tests
g++ -std=c++17 -O1 -g -ffunction-sections -fdata-sections -DVK_NO_PROTOTYPES \
 -Isrc -Ithird_party/volk -I"$headers" "${flags[@]}" tests/gfx_ownership_tests.cc \
 -Wl,--gc-sections -ldl -o build/gfx-ownership-tests
build/gfx-ownership-tests
