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
