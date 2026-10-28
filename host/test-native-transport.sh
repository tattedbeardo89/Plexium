#!/usr/bin/env bash
# Execute the production GnuTLS/socket adapter on Linux; this is not a PS5 run.
set -euo pipefail
cd "$(dirname "$0")/.."
headers=${VULKAN_HEADERS:-/workspace/deps/Vulkan-Headers/include}
make -j2 all >/dev/null
g++ -std=c++17 -O1 -g -Wall -Wextra -Werror -DVK_NO_PROTOTYPES -DTEST_PS5_TRANSPORT \
 -Isrc -Ithird_party -Ithird_party/volk -I"$headers" \
 src/plex/transport_ps5.cc tests/transport_probe.cc build/obj/src/plex/client.o build/obj/third_party/tinyxml2/tinyxml2.o \
 $(pkg-config --cflags --libs gnutls) -pthread -o build/native-transport-probe
python3 tests/test_transport.py native-transport-probe
