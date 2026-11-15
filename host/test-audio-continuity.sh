#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out=build
[[ ${SANITIZE:-0} != 1 ]] || out=build/san
make -j"${BUILD_JOBS:-2}" SANITIZE="${SANITIZE:-0}" "$out/audio-continuity-tests"
for spec in 'm4a aac' 'mp3 libmp3lame' 'flac flac' 'ogg libvorbis'; do
 read -r ext codec <<< "$spec"
 ffmpeg -hide_banner -loglevel error -y -f lavfi -i 'sine=frequency=997:sample_rate=48000' -t 0.65 -c:a "$codec" -ac 2 "$out/continuity.$ext"
done
ffmpeg -hide_banner -loglevel error -y -f lavfi -i 'sine=frequency=997:sample_rate=48000' -t 8 -c:a aac -ac 2 "$out/continuity-control.m4a"
mkdir -p artifacts/logs
"${CONTINUITY_BINARY:-$out/audio-continuity-tests}" "$PWD/$out" "${CONTINUITY_CYCLES:-16}" "${CONTINUITY_SECONDS:-0}" "${CONTINUITY_REPORT:-artifacts/logs/audio-continuity${SANITIZE:+-san}.json}"
