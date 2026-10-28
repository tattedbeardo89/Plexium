#!/usr/bin/env bash
# Use an already prepared, verified native toolchain. Never fetch unverifiable binaries.
set -euo pipefail
cd "$(dirname "$0")/.."
: "${PS5_VULKAN:?Set PS5_VULKAN to the pinned, prepared PS5 Vulkan checkout}"
: "${VLC_PS5_WORK:?Set VLC_PS5_WORK to the verified Plexium-only static dependency build}"
[[ "$VLC_PS5_WORK" == *plexium* ]] || { echo 'Use a separate Plexium dependency/output directory' >&2; exit 2; }
mkdir -p artifacts/logs
WITH_DEMO=0 bash ps5/build-title.sh
python3 tools/check-package.py dist/PPSA85421
python3 ci/validate-native.py dist/PPSA85421 "$VLC_PS5_WORK/build-title/llvm-pie.elf" "$PS5_VULKAN/build/host/ps5-native-tool"
python3 tools/package-title.py dist/PPSA85421 artifacts/Plexium-PPSA85421.zip
cp "$VLC_PS5_WORK/build-title/llvm-pie.elf" artifacts/Plexium-native-debug.elf
cp "$VLC_PS5_WORK/build-title/elf-layout.txt" "$VLC_PS5_WORK/build-title/symbols.txt" "$VLC_PS5_WORK/build-title/plexium.map" artifacts/
(cd artifacts && sha256sum Plexium-PPSA85421.zip Plexium-native-debug.elf > SHA256SUMS)
