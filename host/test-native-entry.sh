#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
headers=${VULKAN_HEADERS:-/workspace/deps/Vulkan-Headers/include}
out=build
flags=()
if [[ ${SANITIZE:-0} == 1 ]]; then out=build/san; flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
mkdir -p "$out"
# Link against the same real ImGui objects used by the application host suites.
obj=build/obj
[[ ${SANITIZE:-0} != 1 ]] || obj=build/obj-san
make -j"${BUILD_JOBS:-2}" SANITIZE="${SANITIZE:-0}" "$obj"/third_party/imgui/{imgui,imgui_draw,imgui_tables,imgui_widgets}.o >/dev/null
g++ -std=c++17 -O1 -g -Wall -Wextra -Werror -Wno-misleading-indentation -DVK_NO_PROTOTYPES \
 -Isrc -Ithird_party -Ithird_party/imgui -Ithird_party/volk -I"$headers" "${flags[@]}" \
 tests/native_entry_tests.cc "$obj"/third_party/imgui/{imgui,imgui_draw,imgui_tables,imgui_widgets}.o \
 -pthread -o "$out/native-entry-tests"
"$out/native-entry-tests"
