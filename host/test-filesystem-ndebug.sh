#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
make -j"${BUILD_JOBS:-2}" all >/dev/null
mkdir -p build/ndebug
for source in state auth session runtime plex; do
 g++ -std=c++17 -O2 -DNDEBUG -D_FORTIFY_SOURCE=3 -Wall -Wextra -Werror -Wno-misleading-indentation \
  -Isrc -Ithird_party -Itests -pthread "tests/${source}_tests.cc" \
  build/obj/src/plex/client.o build/obj/third_party/tinyxml2/tinyxml2.o \
  build/obj/src/app/session.o build/obj/qrcodegen.o -o "build/ndebug/$source-tests"
 "build/ndebug/$source-tests"
done
gcc -O2 -DNDEBUG -D_FORTIFY_SOURCE=3 -Wall -Wextra -Werror -Isrc tests/startup_log_tests.c -o build/ndebug/startup-log-tests
build/ndebug/startup-log-tests
