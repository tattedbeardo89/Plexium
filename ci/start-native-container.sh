#!/usr/bin/env bash
# Recreate processes with the current task proxy and CA; retain build files in /workspace.
set -euo pipefail
cd "$(dirname "$0")/.."
image=plexium-ps5:prepared
expected=$(cat ci/native-image-id.txt)
actual=$(docker image inspect --format '{{.Id}}' "$image" 2>/dev/null || true)
if [[ -z $actual ]]; then
 (cd /workspace/deps && sha256sum --check plexium-ps5-prepared.tar.sha256)
 docker load -i /workspace/deps/plexium-ps5-prepared.tar
 actual=$(docker image inspect --format '{{.Id}}' "$image")
fi
[[ $actual == "$expected" ]] || { echo 'Prepared image identity mismatch' >&2; exit 2; }
name=plexium-native-$(date +%s)-$$
docker run -d --name "$name" \
 --mount type=bind,src=/workspace,dst=/workspace \
 --mount type=bind,src=/etc/ssl/certs/ca-certificates.crt,dst=/run/ca-bundle.pem,readonly \
 -e HTTP_PROXY -e HTTPS_PROXY -e NO_PROXY -e http_proxy -e https_proxy -e no_proxy -e ALL_PROXY -e all_proxy \
 -e SSL_CERT_FILE=/run/ca-bundle.pem -e GIT_SSL_CAINFO=/run/ca-bundle.pem -e CURL_CA_BUNDLE=/run/ca-bundle.pem \
 -w /workspace/Plexium "$image" sleep infinity >/dev/null
printf '%s\n' "$name"
