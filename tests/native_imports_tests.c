#include "check.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include "native_imports.h"
#include <stdio.h>
__asm__(".weak plexium_absent_import");
int main(void) {
    TEST_REQUIRE(PLEX_NATIVE_IMPORT_READY(puts));
    TEST_REQUIRE(!PLEX_NATIVE_IMPORT_READY(plexium_absent_import));
    puts("PASS: RIP-relative GOT checks recognize resolved and absent imports without external files");
}
