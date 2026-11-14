# Native client feature completion matrix

This matrix describes production source and automated acceptance, not a released or
physically accepted application. `ci/HARDWARE-HOLD` remains. No private PMS, account,
network or console is used for this milestone. Exact-commit CI/artifacts are recorded
in draft PR #2; NATIVE-COMPILED requires a successful matching native manifest.

| Area | Implemented production behavior | Automated evidence / remaining boundary |
| --- | --- | --- |
| Authentication / Plex Home | Device linking, restored primary validation, protected-profile PINs, avatars, fresh profile permissions, context isolation and sign-out | Existing auth/session/restore regressions retained. REAL-PMS-TESTED and PS5-TESTED remain pending. |
| Server connection | Authorized discovery, HTTPS identity/library validation, scoped remembered server/route, explicit switching and connection diagnostics | Host fixtures/TLS adapters. Automatic background reconnect is partial; explicit diagnostics/reselect/retry are available. |
| Home | Progressive bounded personal Home plus server-authored On Deck, collections, recommendations, recently played and other promoted hubs when supplied by Plex | Closed base rows remain compatible; extended mode preserves additional authorized hubs, profile-scoped warm cache, partial/offline feedback. No synthetic recommendations or independent database. |
| Movies / television | Posters/backdrops, title/year/runtime/ratings/genres/description/cast, season/episode children, watched markers, versions/tracks, Play/Resume/Restart and explicit watched/unwatched | Production UI and Session fixtures. Watched writes require a user action; failed decoding does not manufacture completion or reset the last observed resume position. |
| Search / browsing | Grouped global hub search, authorized library scope, explicit suggestions, bounded fallback/results/cache, alphabetical/recent/released/unwatched/genre/collections and large pages | 3700-item fixture, unauthorized section filtering, keyboard/controller and raster checks. Suggestions are explicitly requested rather than network calls per key. Provider ranking/suggestions remain Plex-authored. |
| Playback controls | Pause, 10s seek, 60s seek, progress/remaining time, Plex chapters, info overlay, named audio/subtitles/Off, quality/version, retry, restart and adjacent episodes | Existing player lifecycle suites plus new production Session controls. Main loop renders observed position without extra timeline writes per frame. Native runtime acceptance remains pending. |
| Negotiation | Conservative 1080p Direct Play, Direct Stream/transcode decisions, audio/subtitle fallback policy, same-session ping/stop, retained failed release | Error messages distinguish unsupported input, network/HTTP negotiation, incompatible transcoder output, player failure and session expiry. Real transcoder/expired-lease cleanup semantics still need external verification. No guessed release or HTTP downgrade. |
| Progress | Private scoped bounded atomic journal, backoff/coalescing/deduplication, interrupted shutdown, fresh-server reconciliation and explicit watched controls | Durable/episode suites retained; new failed-decoder and captured music-Stop regressions. Server owns watched thresholds; explicit watched changes discard stale pending updates for that item. |
| Episodes | Plex order, next unwatched countdown/cancel/navigation cancellation, fresh next resume, intro/credits markers, manual previous/next and prior-player release | Fixture and actual host VLC end/reopen scenarios. No independently generated episode ordering. |
| Music | Artists/albums/tracks/audio playlists, complete bounded album/playlist queues (2000 tracks), fresh per-track media, shuffle/repeat, next/previous, progress, background browsing and mini-player | Production Session fixtures and actual AAC host VLC queue/controls/end/repeat/reopen. Queue is in memory, not restored across app restarts. Unsupported audio transcoding remains gated by its unverified full lifecycle; no unchecked helper is enabled. Gapless playback, queue editing and richer artist-specific browsing remain future work. |
| Live TV guide | Authorized paginated lineup, logos/numbers/programs/descriptions, local scoped favorites/filter, Mini Guide, Now/time navigation and offline states | Existing 640/4000-channel, expired/repeated-page and screenshot fixtures retained. |
| DVR session | Worker-owned gated controller, ownership/recovery, last/switch, stop/shutdown retention and production request shapes | Fixture-only consumer grants. Production tuning remains BLOCKED; no runtime override. No metadata GET is claimed as a verified consumer heartbeat. |
| Recordings | Read-only rules, schedule, completed list/description and opening completed library metadata | Recording requests/cancellation remain disabled. Permissions and live provider behavior are unverified. No separate DVR/tuner/ErsatzTV backend. |
| Settings / appearance | Account/profile/sign-out, server/diagnostics, quality/direct-play/languages/autoplay, themes, fitted UI safe area, artwork/time controls, actual version/SHA/source date/runtime and installed notices viewer | Atomic owner-only schema-1 preferences per account/profile/server. Unknown schema preserved. No tokens, PINs, addresses or playback URLs in diagnostics. Host build identity regenerates when HEAD changes; native identity uses pinned build stamps. |
| Artwork / metadata memory | Existing scoped 64 MiB GPU / bounded disk cache and owned workers preserved; large media metadata excluded from detail/page caches above 512 KiB; no render-thread I/O | Existing artwork/GPU allocation/timeout/retention tests retained. Context caches remain count-bounded. Further byte-budgeting of history and background prefetch remains an optimization opportunity. |
| Native UI | Shared production canvas, measured text fitting, safe-area/1080p/4K layout, avatar/art aspect handling, stable focus and responsive worker loading | 26 nonblank distinct native-view screenshots plus controller scenarios. This is HOST-TESTED rendering, not console presentation acceptance. |
| 4K preparation | Existing 8/10-bit substrate preserved; overflow-safe surface-only memory estimates and host budget pressure checks | NOT functional 4K acceptance. No decoder eviction, arena free or speculative driver/kernel call added. See 4K-SOURCE-REVIEW.md. |
| Native delivery | Separate startup-only and full application modes, source/debug/signing/ZIP/license/hold checks | Both require matching exact-head CI. Full source compilation is not REAL-PMS-TESTED or PS5-TESTED behavior. |

## Evidence boundaries

IMPLEMENTED means reachable production application source, not an unused helper.
HOST-TESTED means deterministic production-component tests or actual system VLC
software decoding, with explicit synthetic URL/metadata mappings. Native UI raster
fixtures run the same View and canvas used by PS5. No unrelated web UI exists.
NATIVE-COMPILED records only successful exact-source compile/link/sign/package checks.
REAL-PMS-TESTED and PS5-TESTED are pending by user instruction; no access is requested.

The original reboot cause, display-engine presentation completion, native audio-close
ABI, decoder cache/arena release contracts, hardware 4K and native playback acceptance
remain unresolved. Safe retention and all existing fault diagnostics remain intact.
