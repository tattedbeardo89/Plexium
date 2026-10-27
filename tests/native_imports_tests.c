// SPDX-License-Identifier: GPL-3.0-or-later
#include "native_imports.h"
#include <assert.h>
#include <stdio.h>
__asm__(".weak plexium_absent_import");
int main(void) {
    assert(PLEX_NATIVE_IMPORT_READY(puts));
    assert(!PLEX_NATIVE_IMPORT_READY(plexium_absent_import));
    puts("PASS: RIP-relative GOT checks recognize resolved and absent imports without external files");
}
