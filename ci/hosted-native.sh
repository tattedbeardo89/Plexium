#!/usr/bin/env bash
# Portable GitHub-hosted build: fixed paths exist only inside this container.
set -euo pipefail
repo=$(cd -- "$(dirname "$0")/.." && pwd)
cache=${PLEXIUM_CI_CACHE_DIR:-$repo/.native-cache}
mkdir -p "$cache/deps" "$cache/plexium-work" "$repo/artifacts/logs"
image=$(python3 -c 'import json,sys;print(json.load(open(sys.argv[1]))["arch_image"])' "$repo/ci/build-lock.json")
name=plexium-ci-${GITHUB_RUN_ID:-local}-$$
finish() {
 result=$?
 trap - EXIT
 docker exec "$name" chown -R "$(id -u):$(id -g)" /workspace/Plexium/artifacts /workspace/deps /workspace/plexium-work >/dev/null 2>&1 || true
 docker rm -f "$name" >/dev/null 2>&1 || true
 exit "$result"
}
trap finish EXIT
# Public system CA trust is mounted read-only; no Codex-specific CA or image cache.
docker run -d --init --name "$name" \
 --mount "type=bind,src=$repo,dst=/workspace/Plexium" \
 --mount "type=bind,src=$cache/deps,dst=/workspace/deps" \
 --mount "type=bind,src=$cache/plexium-work,dst=/workspace/plexium-work" \
 --mount type=bind,src=/etc/ssl/certs/ca-certificates.crt,dst=/run/ca-bundle.pem,readonly \
 -e HTTP_PROXY -e HTTPS_PROXY -e NO_PROXY -e http_proxy -e https_proxy -e no_proxy \
 -e SSL_CERT_FILE=/run/ca-bundle.pem -e CURL_CA_BUNDLE=/run/ca-bundle.pem -e GIT_SSL_CAINFO=/run/ca-bundle.pem \
 -w /workspace/Plexium "$image" sleep infinity >/dev/null
docker exec "$name" bash ci/prepare-container.sh
docker exec "$name" bash ci/hosted-native-inner.sh
