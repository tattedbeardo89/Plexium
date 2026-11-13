#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out=build
[[ ${SANITIZE:-0} != 1 ]] || out=build/san
make -j"${BUILD_JOBS:-2}" SANITIZE="${SANITIZE:-0}" "$out/host-vlc-tests"
ffmpeg -hide_banner -loglevel error -y -f lavfi -i testsrc2=size=1920x1080:rate=24 -f lavfi -i sine=frequency=440:sample_rate=48000 -t 18 -c:v libx264 -preset ultrafast -crf 28 -c:a aac -ac 2 "$out/synthetic-1080p.mp4"
cat > "$out/synthetic-captions.srt" <<'CAPTIONS'
1
00:00:00,000 --> 00:00:07,500
Synthetic caption fixture
CAPTIONS
ffmpeg -hide_banner -loglevel error -y -f lavfi -i testsrc2=size=1280x720:rate=24 -f lavfi -i sine=frequency=440:sample_rate=48000 -f lavfi -i sine=frequency=880:sample_rate=48000 -i "$out/synthetic-captions.srt" -t 8 -map 0:v -map 1:a -map 2:a -map 3:s -c:v libx264 -preset ultrafast -crf 30 -c:a aac -ac 2 -c:s srt -metadata:s:a:0 language=eng -metadata:s:a:1 language=spa -metadata:s:s:0 language=eng -disposition:a:0 default -disposition:a:1 0 -disposition:s:0 0 "$out/synthetic-tracks.mkv"
if [[ ${SANITIZE:-0} == 1 ]]; then
 mkdir -p artifacts/logs
 LD_DEBUG=files LD_DEBUG_OUTPUT="$PWD/artifacts/logs/host-vlc-loader" "$out/host-vlc-tests" "$PWD/$out/synthetic-1080p.mp4" "$PWD/$out/synthetic-tracks.mkv" "$PWD/$out/synthetic-captions.srt"
else
 "$out/host-vlc-tests" "$PWD/$out/synthetic-1080p.mp4" "$PWD/$out/synthetic-tracks.mkv" "$PWD/$out/synthetic-captions.srt"
fi
