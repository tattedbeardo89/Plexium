#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out=build
[[ ${SANITIZE:-0} != 1 ]] || out=build/san
make -j"${BUILD_JOBS:-2}" SANITIZE="${SANITIZE:-0}" "$out/plexium-server-check"
"$out/plexium-server-check"
