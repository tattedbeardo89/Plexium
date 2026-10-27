# Exact 0.1 / 0.2 native comparison — 2026-10-27

The user reports 0.1 launches correctly and 0.2 fails before any visible Plexium UI, with the last attempt followed by reboot, safe mode and system-software recovery. The installation hold remains. No console was contacted or asked to reproduce the failure.

## Reproduced endpoints

Both the working 0.1 and failing 0.2 signed executables **and complete unstripped debug ELFs** were reproduced byte for byte from their original application source. [Machine-readable evidence](native-comparison-evidence.json) records hashes and scope.

| Native source | SELF SHA-256 | Debug ELF SHA-256 |
| --- | --- | --- |
| 0.1 `2fbce98d89f830bd0df67f321fa7262296b6f643` | `1bcda8075604fbe35590505f3b3b2bc146f67356bf5cd35906d1568a36438dd1` | `554a7d5e55e7a61ecd5b1e9ac2873d00d373ec39dc474d6487af6a3df46412d2` |
| 0.2 `75d7ecf672ed79ab165a879a9a059b899801ae56` | `799a6905051bd3f6f1a6d59f4a3d8824de2e664469ea42623149cf838d9531d8` | `2b18314af5814378eb862c34e5e7d203f71204b457a5801c9e6870b0809a7b5a` |

The comparison uses the pinned prepared compiler/runtime/Vulkan/VLC files, canonical `/workspace/Plexium` paths and four linker threads. An exact Git-object check covered all 250 original 0.1 source files. Two local dependency differences were eliminated in isolated copies: libass was rebuilt without its auto-detected iconv feature, matching both published executables; VLC's single same-length compile-time string was normalized from `Oct 26 2026 02:16:34` to their shared `Oct 26 2026 15:35:18`. Cached dependency files were not modified. The latter is explicitly a diagnostic metadata adjustment, not a shipped binary fix. Full SELF and debug-ELF equality confirms the resulting endpoints, including native code and data.

## Controlled source layers

Four source layers were built and signed: original 0.1; added protocol/metadata changes; added browsing/session/UI changes with original main source; and full 0.2 entry-point wiring. Intermediate layers are experiments, not commits/releases. Their identities and source hashes are retained in the workspace manifests. All compile/link/sign/layout checks passed. No stage was executed on PS5; there is no behavioral pass/fail oracle from which to name a first crashing commit.

Static comparisons found identical constructor names/order (32), identical imported symbol sets (481), valid load alignment/entry bounds, writable relocation destinations and in-image relative relocation values. The original reports each match their own executable's 481 import slots. Cross-version reports match zero slots, illustrating the external-offset hazard already removed by the startup review; the published ZIPs themselves have correct pairings.

Native DWARF showed one consistent layout per type across all recorded definitions. `plex::Item` grew from 616 to 736 bytes; Model from 904 to 1200; Session from 1360 to 1840; View from 144 to 304. No mixture of old/new layouts was found in the published 0.2 binary. `_start`, `_init`, all constructors and the graphics/platform/player initialization functions retain their code sizes; this does not establish behavior or prove every instruction identical after relocation.

## Measured startup resource regression and correction

| Source layer | `main` stack reservation | Shared `View::frame` reservation |
| --- | ---: | ---: |
| Working 0.1 | 3744 | 1480 |
| Protocol/metadata | 3968 | 1480 |
| Browsing/session/UI, old main source | 4576 | 2504 |
| Published 0.2 | 4544 | 2504 |

These are immediate x86-64 prologue reservations, excluding saved registers and callee stacks. The shared UI function reserves its full frame even on Welcome/Profiles. The new cast renderer's full Item temporary therefore affects startup stack allocation before that panel is selected. The observed growth is real; the console's actual stack limit and failing stack pointer are unknown, so exhaustion is not established as the reboot cause.

The source correction stores application state and a reusable snapshot on the heap, copies the model and authorization context together under one lock, and puts detail rendering in a separate non-inlined function. This retains the 0.2 artwork, cast/related, Recommended browsing, caching, overlay, audio and experimental Free TV paths. Tests verify snapshot independence/capacity reuse and existing shared-view behavior. A native frame regression gate compares the combined immediate main/shared-view reservations against the measured 0.1 budget (5224 bytes). It is not a full call-chain stack bound or a hardware acceptance test.

The earlier startup fixes remain: no external import offsets/assumed base reads, no automatic helper/whitelist/sandbox-change requests, valid stderr after failed log opening, restricted fatal-signal operations, and checked/bounded frame failure handling. [Startup audit](STARTUP-AUDIT.md) records their limits.

## Reproducing the comparison

`tools/bisect-native.py` exports source archives without changing the product checkout or creating worktrees. It verifies the prepared image, records native dependency hashes, uses one network-disabled container, builds the source layers and retains results under `artifacts/native-bisection`. It refuses to overwrite a prior experiment. Optional `PLEXIUM_BISECT_OUT`, `PLEXIUM_BISECT_LIBASS`, `PLEXIUM_BISECT_VLC_TIME` and `PLEXIUM_BISECT_REUSE` select a separate output, verified no-iconv libass archive, explicit timestamp normalization, and previously verified app objects respectively. Every reuse validates source stamps/hashes. `tools/native-diff-audit.py` reads ELF layout, constructor and import/relocation evidence. No script deploys, launches a PS5 executable or publishes files.

## Remaining limit

The comparison has found and corrected a startup resource regression, but **has not identified the failing instruction or proved the cause of reboot/recovery**. Failure before visible UI can include hidden frames before splash dismissal. Native fault/module evidence or a compatible execution environment is needed for that attribution. The reviewed build remains held from installation, and the original 0.1 package is preserved. Free TV authenticated/native playback and full audio queues remain unfinished acceptance work.
