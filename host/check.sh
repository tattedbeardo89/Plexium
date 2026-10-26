#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
make -j"${BUILD_JOBS:-2}" SANITIZE="${SANITIZE:-0}" all
out=build
[[ ${SANITIZE:-0} != 1 ]] || out=build/san
for suite in runtime-tests session-tests protocol-tests auth-tests home-tests home-feed-tests; do "./$out/$suite"; done
python3 tests/test_transport.py "$(if [[ $out == build ]]; then echo transport-probe; else echo san/transport-probe; fi)"
"./$out/plexium-render" "$out"
python3 tests/verify_frames.py "$out"
python3 tests/branding_tests.py
