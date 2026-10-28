// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
/* Read the linked image's own GOT, using RIP-relative addressing. Never derive
 * live pointers from imports.txt or an assumed load base. The native converter
 * resolves these slots just like ordinary imported calls. */
#define PLEX_NATIVE_IMPORT_READY(symbol) \
    __extension__ ({ uintptr_t plex_import_value; \
        __asm__ volatile("movq " #symbol "@GOTPCREL(%%rip), %0" \
                         : "=r"(plex_import_value)); \
        plex_import_value != 0; })
