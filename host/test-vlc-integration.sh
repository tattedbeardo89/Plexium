#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out=build
[[ ${SANITIZE:-0} != 1 ]] || out=build/san
make -j"${BUILD_JOBS:-2}" SANITIZE="${SANITIZE:-0}" "$out/host-vlc-tests"
ffmpeg -hide_banner -loglevel error -y -f lavfi -i testsrc2=size=1920x1080:rate=24 -f lavfi -i sine=frequency=440:sample_rate=48000 -t 18 -c:v libx264 -preset ultrafast -crf 28 -c:a aac -ac 2 "$out/synthetic-1080p.mp4"
"$out/host-vlc-tests" "$PWD/$out/synthetic-1080p.mp4"
