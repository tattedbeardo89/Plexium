# Validation record — 2026-10-25

Plexium is a **tested development checkpoint**, not a completed installable PS5 release. No host simulation is reported as console playback.

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
| Target prerequisites | PASS: 19 recipes | NASM, compatibility, zlib, FFmpeg, dav1d, EBML, Matroska, dvbpsi, FriBidi, GMP, Nettle, GnuTLS, SMB2, UPnP, libxml2, dvdread, dvdnav, libarchive, iconv |
| Source archives | 21 PASS / 1 BLOCKED | All 22 have published pinned digests; FreeType 2.13.3 source mirror denied |
| Blu-ray recipe | BLOCKED | Its required FreeType dependency is absent; dependent HarfBuzz/libass chain also unbuilt |
| Full `ci/build-native.sh` | BLOCKED, exit 2 | Static VLC core is absent until font-dependent prerequisites build; no final application link/sign step |
| Native package inspection | NOT PRODUCED | Inspector correctly refuses absent eboot.bin; no install ZIP or native application ELF exists |
| Fresh native container and image archive | PASS | Compiler/CA access, HTTPS to GitHub, prepared image ID, archive checksum; no live process assumed to survive |
| Host CLI smoke/restart | PASS | Private directory 0700, identity file 0600, identity retained; no account linked |
| Remote CI / PR | UNRUN | Workflow files are local; no push/PR/merge/public release |
| Live Plex / physical PS5 / Windows PowerShell | UNRUN | No user account, PMS media, console or Windows execution was used |

Logs are in `artifacts/logs`. The final host, sanitizer, native-object, native-syntax, native-transport and source-verification logs are separate. Early sanitizer attempts used stale objects after a header changed during a build; a clean rebuild resolved that mismatch, and the final normal/instrumented workflows passed after the final source changes. Assertions were retained. No TLS, package signature, SDK checksum, runtime digest or dependency checksum was bypassed.

The session fixture covers restore, profiles/PIN, single authorized server, libraries, closed Home ordering, metadata, playback-port dispatch, pause timeline state, preserving captured stop progress, invalid selections, failed-profile context clearing and sign-out. The view checks correct item activation, back/focus restoration, R1 registry, immediate draw construction and 4K layout construction. These are not live account/decoder tests.

## Performance evidence

The final ordinary host run constructed 100 shared-view frames in **0.614244 ms**; the instrumented run took **5.13844 ms**. These numbers exclude networking, rasterization, GPU presentation and video/audio decode. Six actual draw-list rasterizations were produced separately at 1280×720. No console startup, 1080p/4K GPU frame rate, decoder performance, audible codec support or before/after PS5 comparison was measured. No comparable PlexStation hardware baseline was collected.

## Remaining product work

The vertical slice is proven only through synthetic playback dispatch plus target object compilation. Real sign-in, PINs, server permissions, media metadata, posters, persistent runtime paths, playback/audio/subtitle/seek behavior, shell icon registration and firmware 13.40 acceptance remain unverified. Complete the native dependency chain and real package first.

Broader requirements remain incomplete: profile avatars in the new view, full sorting/filtering/pagination, grouped global search, full cast/related/collection/season layouts, configurable hero policy, Watchlist, dedicated music/audiobooks, personal DVR, transcode negotiation, chapter/next-episode UX, durable timeline reconciliation, screensaver, renderer stress/performance and console artwork acceptance. Some supporting protocol helpers are present but do not establish working product features. Free cloud Live TV is intentionally omitted.
