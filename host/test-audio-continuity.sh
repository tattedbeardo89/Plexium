#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out=build
[[ ${SANITIZE:-0} != 1 ]] || out=build/san
make -j"${BUILD_JOBS:-2}" SANITIZE="${SANITIZE:-0}" "$out/audio-continuity-tests"
python3 tools/generate-mixed-audio.py "$out"
mkdir -p artifacts/logs
"${CONTINUITY_BINARY:-$out/audio-continuity-tests}" "$PWD/$out" "${CONTINUITY_CYCLES:-16}" "${CONTINUITY_SECONDS:-0}" "${CONTINUITY_REPORT:-artifacts/logs/audio-continuity${SANITIZE:+-san}.json}"
