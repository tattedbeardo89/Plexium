#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
headers=${VULKAN_HEADERS:-/usr/include}
vlc_headers=${VLC_HEADERS:-/usr/include}
out=build
flags=()
if [[ ${SANITIZE:-0} == 1 ]]; then out=build/san; flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
mkdir -p "$out"
g++ -std=c++17 -O1 -g -ffunction-sections -fdata-sections -DVK_NO_PROTOTYPES \
 -I"$vlc_headers" -I"$vlc_headers/vlc/plugins" -I"$headers" -Isrc -Ithird_party/volk \
 "${flags[@]}" tests/player_lifecycle_tests.cc src/mpvshader.cc -Wl,--gc-sections -ldl -pthread -o "$out/player-lifecycle-tests"
"$out/player-lifecycle-tests"

gcc -std=c11 -O1 -g -ffunction-sections -fdata-sections -D_GNU_SOURCE \
 -I"$vlc_headers/vlc/plugins" "${flags[@]}" tests/decoder_release_tests.c \
 -Wl,--gc-sections -pthread -o "$out/decoder-release-tests"
"$out/decoder-release-tests"
