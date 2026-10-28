# Validation record — 2026-10-25

Plexium is a **real signed, package-verified native development checkpoint**, not a complete feature release or hardware-accepted application. No host simulation is reported as console playback.

| Check | Result | Scope |
|---|---|---|
| `bash host/check.sh` | PASS | Six C++ suites: runtime, session, protocol (74 checks), authentication, Plex Home, Home feed; ten libcurl HTTP/TLS cases; six rendered screens; controller/focus and deterministic branding checks |
| `SANITIZE=1 bash host/check.sh` | PASS | Same selected host workflow under AddressSanitizer/UBSan; separate instrumented objects and binaries |
| `bash host/test-native-transport.sh` | PASS | Ten real loopback HTTP/TLS scenarios using the production GnuTLS/socket adapter on Linux; not PS5 ABI execution |
| `bash host/check-native-source.sh` | PASS | Strict host syntax of entry, texture boundary, shared application, Plex client, native transport and artwork |
| `docker exec <fresh-native-session> bash ci/check-native-objects.sh` | PASS | Seven actual `x86_64-sie-ps5` target objects: main, session, view, textures, client, native transport, artwork; debug information retained |
| Open SDK install | PASS | Original v0.42 archive digest verified; fork b83202be73e930050e00de0f1b4d9ec46c0391bf installed |
| Runtime rebuild | PASS | Two byte-identical builds; original raw/signed hashes unchanged and verified |
| Release RADV + reference HDR patch | PASS | Mesa 1ba9874f478657ccf6da72ce55f0260164cc0bd0; offline patch cache checked against existing expected hash; repeat bootstrap passed |
| Target prerequisites | PASS: 23 recipes | NASM, compatibility, zlib, FFmpeg, dav1d, EBML, Matroska, dvbpsi, FriBidi, GMP, Nettle, GnuTLS, SMB2, UPnP, libxml2, dvdread, dvdnav, libarchive, iconv, FreeType, HarfBuzz, libass, Blu-ray |
| Source archives | 22 PASS | All 22 have published pinned digests and downloaded bytes verified; initial FreeType denial resolved |
| Font/Blu-ray chain and static VLC | PASS | Required fonts/text dependencies and 111 static VLC plugins built |
| Full `ci/build-native.sh` | PASS | Native compile/link, signed SELF integrity, static plugin/import checks, package validation and matching debug ELF |
| Native installation ZIP | PASS | Correct PPSA85421 identity, PNG/DDS assets, 100 notices, CRC, 755/644 modes, matching payload; repeated packaging was byte-identical |
| Fresh native container and image archive | PASS | Compiler/CA access, HTTPS to GitHub, prepared image ID, archive checksum; no live process assumed to survive |
| Host CLI smoke/restart | PASS | Private directory 0700, identity file 0600, identity retained; no account linked |
| Remote CI / PR | PASS | [Native build and four uploads](https://github.com/tattedbeardo89/Plexium/actions/runs/40347283259); [independent GitHub downloads/identity checks](https://github.com/tattedbeardo89/Plexium/actions/runs/40351502414); PR #1 remains open, with no merge/public release. |
| Live Plex / physical PS5 / Windows PowerShell | UNRUN | No user account, PMS media, console or Windows execution was used |

Logs are in `artifacts/logs`. The final host, sanitizer, native-object, native-syntax, native-transport and source-verification logs are separate. Early sanitizer attempts used stale objects after a header changed during a build; a clean rebuild resolved that mismatch, and the final normal/instrumented workflows passed after the final source changes. Assertions were retained. No TLS, package signature, SDK checksum, runtime digest or dependency checksum was bypassed.

The session fixture covers restore, profiles/PIN, single authorized server, libraries, closed Home ordering, metadata, playback-port dispatch, pause timeline state, preserving captured stop progress, invalid selections, failed-profile context clearing and sign-out. The view checks correct item activation, back/focus restoration, R1 registry, immediate draw construction and 4K layout construction. These are not live account/decoder tests.

## Performance evidence

The final ordinary host run constructed 100 shared-view frames in **0.614244 ms**; the instrumented run took **5.13844 ms**. These numbers exclude networking, rasterization, GPU presentation and video/audio decode. Six actual draw-list rasterizations were produced separately at 1280×720. No console startup, 1080p/4K GPU frame rate, decoder performance, audible codec support or before/after PS5 comparison was measured. No comparable PlexStation hardware baseline was collected.

## Remaining product work

The vertical slice has synthetic session/playback-dispatch coverage and a real compiled/linked/signed native application. Real sign-in, PINs, server permissions, media metadata, posters, persistent runtime paths, playback/audio/subtitle/seek behavior, shell icon registration and firmware 13.40 acceptance remain unverified. Use the generated native package and matching symbols for controlled console acceptance; those results are the next evidence gate.

Broader requirements remain incomplete: profile avatars in the new view, full sorting/filtering/pagination, grouped global search, full cast/related/collection/season layouts, configurable hero policy, Watchlist, dedicated music/audiobooks, personal DVR, transcode negotiation, chapter/next-episode UX, durable timeline reconciliation, screensaver, renderer stress/performance and console artwork acceptance. Some supporting protocol helpers are present but do not establish working product features. Free cloud Live TV is intentionally omitted.

## Original local native artifact identity

Native source commit: `36abbd8b5a9bbf9c0afda47af1142ec702669ab2`. ELF build ID: `169ce79e246e3e418ac7bfd49aefbab8ac17f9aa`. The full commit was found in the debug ELF. SELF SHA-256: `c7492c38e541159b59c00c2c968b651deb246747715a6a4c3357e2c70525192b`. Installation ZIP SHA-256: `1cd9500d2f0b58c8c5952782e96d0ce8c1b266c54afe4e184c29ea0ba22374ed`. Debug ELF SHA-256: `601f3ee3a682d5da6502b6715658069ced5f50b0691c590745b74273ffb3175d`.

The initial native link failed because the retained platform crash handler expected a bridge from the old UI. The new entry implements the real atomic checkpoint bridge. A later provenance check caught an unversioned label caused by container Git ownership; the exact-checkout safe-directory read fixed it, and the final native rebuild/package validation passed. These corrected build issues are not physical PS5 test results.

## Original 0.1 rollback candidate

Native source: `2fbce98d89f830bd0df67f321fa7262296b6f643`. ELF build ID: `3cf535abec047e9cc242864ec37d24ecf2f54268`. [Native CI](https://github.com/tattedbeardo89/Plexium/actions/runs/40347283259) and [independent download verification](https://github.com/tattedbeardo89/Plexium/actions/runs/40351502414) passed. The draft release remains private to authorized GitHub users, unpublished and marked prerelease. GitHub release asset digests match the downloaded Actions files.

- `Plexium-PPSA85421.zip`: SHA-256 `4fa600c9066ca0759e6a480063222725203a2195e14ef4096ba680242f8929d0`
- `Plexium-native-debug.elf`: SHA-256 `554a7d5e55e7a61ecd5b1e9ac2873d00d373ec39dc474d6487af6a3df46412d2`
- `Plexium-native-source.tar.gz`: SHA-256 `c5d1d17e50e642511e9cc04eda946e0ae368786f0c87cf9368c13380c83fcadb`
- `Plexium-source.bundle`: SHA-256 `b2ddef5df0f020abb24faa96929c1488c413d173b7c5913a532d30174b224bce`

Physical PS5 firmware 13.40/live Plex acceptance remains unrun. The earlier local hashes above describe a separate historical candidate.

## Browsing update validation — 2026-10-26

The user reports the original candidate shows the correct PS5 home icon, a generally working UI and working video playback. This is user-reported acceptance of that original candidate, not a developer-run hardware test or acceptance of the new update.

The updated host suites and shared-view regressions pass, including avatar callback wiring, detail poster/backdrop requests, multiple cast portraits, automatic video overlay hiding/waking/paused-buffering behavior, 60-item pages for a 3,700-item fixture, scoped warm caches, incremental Home publication, reduced populated-hub requests, preserved stopped resume positions and scoped non-DRM cloud-channel dispatch. ASan/UBSan and both ten-scenario HTTP/TLS adapters also passed during this update. Final native CI reruns the host/sanitizer checks against its exact source commit.

The separate 0.2 candidate passed [native CI](https://github.com/tattedbeardo89/Plexium/actions/runs/40399618166) and [independent download verification](https://github.com/tattedbeardo89/Plexium/actions/runs/40402550151). Compiled source is `75d7ecf672ed79ab165a879a9a059b899801ae56`; ELF build ID is `ca072e3b015b8caf5e14be67b76d67ca64149227`. Installation ZIP SHA-256: `08ce07a90f7c009a9be09265bcfd7c18c496b82b3143b442d0d93a4bc26528af`. Original 0.1 remains unchanged. The optional anonymous provider probe returned registration HTTP 404 and channel-list HTTP 401, with zero channels; this is not authenticated service acceptance. Experimental Free Live TV still requires real profile/region/native audio-video/ad/session acceptance; metadata compatibility reports are distinct from stream tests.

## Startup review after reboot report — 2026-10-27

The user reports 0.2 launch failure, reboot, safe mode and system-software recovery after the executable-only comparison. The earlier installation-only diagnosis is superseded. Root cause remains unresolved; do not repeat the device test. [Startup audit](STARTUP-AUDIT.md) records concrete corrections, preserved 0.2 features and passing offline/native checks for source `46b736a20e7833d82beabe9467545da610da4256`. `ci/HARDWARE-HOLD` blocks candidate replacement. No new package is hardware accepted or recommended for installation. Original 0.1 remains unchanged.


Final offline startup correction: native source `09671e31df3e99a5257a58faf7e8fc32b7917ac7` passed full compile/link/sign/package, exact source/debug stamp, ZIP payload/CRC/modes and stack-budget checks. Immediate entry/UI reservations measured 2616 bytes (0.1: 5224), worker/dispatch 1952 (0.1: 2272), and entry/send 2200 (0.1: 4696). These exclude callee frames and saved registers. See [native comparison](NATIVE-COMPARISON.md). No device execution or safe-launch claim; reboot attribution remains unresolved and hardware hold remains.

The final successful-worker regression fixture and full host suites passed normally and with ASan/UBSan after this correction, including independent reusable model/context snapshots and retained vector capacity. These are local results; no fresh remote CI completion is claimed.
