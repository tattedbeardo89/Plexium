# Full-client readiness audit

This is an audit of reachable application behavior, not a production release or
hardware safety certification. The baseline is `7d5df212926ef9dc44c81f821cbf1258cd03c2c5`.
Exact final source identity, CI runs, held package hashes and measurement artifacts
are recorded in existing draft PR #2. No real PMS, private network or console is used.

## Reachability and evidence

“COMPLETE” below applies to the stated implemented scope. Every native feature is
also **BLOCKED — PS5 required** for physical input, presentation, audio/decoder and
firmware acceptance. Host fixtures establish authorized protocol handling, not live
service acceptance. Automated regression names refer to `tests/*_tests.cc` and the
commands in `host/check.sh`, `host/test-vlc-integration.sh`, and the optional soak.

| Area / classification | Reachable production path and behavior | Failure / context / ownership evidence and boundary |
| --- | --- | --- |
| Authentication — COMPLETE — Host-tested | `View::frame` Welcome/Linking/Pin → Link/Poll/Profile; `Session::restore_command`, `select_profile`; `plex/auth.h`, `home.h` | auth/session/restore suites: expiry/cancel, primary restore, protected PIN, profile context clearing. BLOCKED — Real PMS required for account service acceptance. |
| Plex Home / profiles — COMPLETE — Host-tested | Profiles avatar grid → Profile; Settings → Home("profiles"); fresh Home permissions and private scoped prefs | home/home-feed, restore/music suites and actual PCM cross-profile isolation. No managed token restored past PIN. |
| Server selection / reconnect — COMPLETE — Host-tested | Servers → Server; Settings → ReconnectServer/Diagnose; `reliability.cc::reconnect_server` validates SAME machine/account/profile | music/durable-progress/TLS suites; six-attempt circuit, three routes; metadata/diagnostics 401/403 now immediately revoke private state. DNS/TLS use real loopback adapter tests and a transport-error category in PCM soak, not real service DNS fault injection. |
| Home — COMPLETE — Host-tested | `plex/home_feed.h`, `Session::home`, native Home rail actions → Details/Library | Progressive authorized hubs, bounded warm cache, errors/empty states; home-feed/client suites. Plex ranking remains server authored. |
| Movies — COMPLETE — Host-tested | Library tabs/page/category → `library`, `listing`, `details_command` → Play/Restart/MarkWatched/Tracks | client/session/playback/track suites; wrong-item rejection, scoped caches, explicit watched writes, failure-state position preservation. |
| TV / episodes — COMPLETE — Host-tested | Native show/season/episode Details → Children; EpisodePrevious/Next/PlayNext; `open_episode` | episode/durable-progress + actual VLC: Plex order, countdown cancel, fresh resume and stop/release. Server watched semantics still need real PMS acceptance. |
| Search — COMPLETE — Host-tested | Search keyboard → Search/Suggest; `features.cc::search_command` → authorized hubs/bounded fallback | client suite: 3700-item paging, grouped search, scope denial. Explicit Suggest, no network per keystroke. No independent search index. |
| Music browse / queue — COMPLETE — Host-tested | Artist/album/playlist/track Details → QueuePlay/QueueAdd; Queue → QueueEdit/Resume; `music.cc` | 10000-track/1200-album, 2000-entry/10000-edit tests; distinct duplicate IDs, scope epochs, current removal and incomplete-stop retention. Raw controller chord tests. |
| Music restoration — COMPLETE — Host-tested | `MusicStore` schema-1 bind/save; `bind_music`, `validate_music`, explicit QueueResume | corrupt/unknown/torn/symlink files, deleted libraries/items and no-autoplay restore; actual PCM restart/profile tests. Scoped private ID-only state, fresh media before open. |
| Background / focus — COMPLETE — Host-tested | Native mini-player → MusicPause/Previous/Next/Open/Queue; `MusicNavigation`, `music_focus_video`, `resume_after_video` | client/music + actual PCM/VLC: navigation retained, one player, video focus policy default-Off restoration; failed stop cannot replace ownership. |
| Video playback / tracks — COMPLETE — Host-tested | Play/Version/Quality/Tracks/SelectAudio/Subtitle/Seek/Chapter/Retry; `open_playback`, `apply_tracks`, `tick_command` | track/playback/episode suites and real local VLC H.264/AAC, HLS and subtitles; production negotiation uses synthetic PMS responses. BLOCKED — Real PMS required for real Direct Stream/transcoder lifecycle. |
| Music transitions — PARTIAL | `music_tick` → revalidated natural MusicNext, single stop/open; one scoped metadata pre-resolution | PCM measurements include codec changes and timeline latency. Natural stopped progress is persisted before reopen and sent after the next player has opened. No second native decoder/port; no sample-continuous gapless claim. Short successful tracks that end between ticks retain their stopped progress; failed inputs do not. |
| Unsupported music conversion — PARTIAL; BLOCKED — Real PMS required | `inspect_audio_fallback` checks official decision identity/output; incompatible named stream keeps current player | strict audio-decision fixtures; no music start, guessed keepalive/release or override. A verified end-to-end consumer contract remains essential source work before enabling conversion. |
| Live TV — COMPLETE — Host-tested for guide; BLOCKED — Real PMS required for tuning | Native Guide/Favorites/MiniGuide → live guide commands; gated `LiveTvSession` | guide/live-session/DVR suites, 640/4000 channels and native frames. Production protocol cannot opt in. No tuner/XMLTV/ErsatzTV backend. |
| DVR recordings — COMPLETE — Host-tested for read-only scope; BLOCKED — Real PMS required | Recordings browser → completed metadata Details | DVR integration validates allowed reads/ownership; recording creation/cancellation disabled. No mutation control is represented as functional. |
| Settings / diagnostics — COMPLETE — Host-tested | Six native sections → Setting/Quality/AutoPlay/Diagnose/Server/Stop/Notices | client/music/preferences tests, fresh diagnostics authorization checks; private schema settings. New release-error frame verifies the Stop action stays accessible and its message visible. Diagnostics are not hardware telemetry. |
| License notices — COMPLETE — Host-tested | System → Notices → OpenNotice → bounded installed catalog/body, Circle return | installed notice path/symlink/size rejection, list/body frames and native packaged licensing checks. No placeholder external site. |
| Memory / workers — COMPLETE — Host-tested for explicit bounds | `model_budget.h`, `reliability.cc::trim_caches`, `AudioQueue`, artwork workers, bounded Runtime queue | byte-pressure tests now account profile/navigation/track/live strings and retained JSON/vector capacity; cache/queue limits are conservative accounting, not total native RSS. Actual PCM resource plateau checks complement ASan/UBSan. |
| Native delivery — COMPLETE — Native-compiled, hardware-unverified only after matching CI | pinned toolchain → compile/link/sign/package; source stamp, debug ID, CRC/modes, licenses, stack guards, hold | Both modes must pass exact-source CI. Earlier results remain historical; latest PR evidence is authoritative. No deployment or stable publication. |
| Watchlist / screensaver / cloud-provider parity — DEFERRED — Nonessential to this scoped milestone | Not represented as complete native workflows; optional cloud provider retains compatibility reporting | No UI mock or unused helper is counted as a working feature. Further product scope needs its own implementation/acceptance. |

## Defects corrected in this milestone

- **DEFECT — Must fix → fixed:** a delayed render-thread MusicNext("ended") could
  advance an already replaced track. Native input now wakes a coalesced Tick, and
  Session rejects end notifications unless its current player still reports Ended.
- **DEFECT — Must fix → fixed:** stopped progress network work and optional server
  health/restoration reads preceded natural queue advancement. The outgoing update
  is durably journaled, networking deferred until after the next open, before a new playing update can replace it, and observed
  ends run before optional health work (health still runs after open to prevent starvation). Manual Stop/release remains synchronous.
- **DEFECT — Must fix → fixed:** metadata/diagnostic authorization failures could
  leave private content visible until the periodic health probe. These failures now
  feed the existing revocation circuit; Details/Diagnose regressions verify it. Explicit recovery restores the fresh authorized profile roster so protected-user selection remains reachable.
- **DEFECT — Must fix → fixed:** Settings rows covered the generic release-error
  banner. Its fitted banner now occupies the top message area; the retry row remains
  reachable. Long banners no longer wrap outside their allocated height.
- **DEFECT — Must fix → fixed:** model byte accounting omitted profile, destination,
  track/live state strings and reserved JSON/vector storage. Those allocations now
  contribute to admission/eviction without changing any budget or native retention.

## Resource ownership audit

`Runtime` has one joined Session worker and at most sixteen pending commands. Player
controls marshal to the existing render thread. Metadata pre-resolution owns one
item, never another decoder or audio output. `MusicNavigation` restores browsed state
without duplicating player ownership. Persistence is bounded (2 MiB / sixteen scopes),
private and atomic. Progress journal reconciliation rejects stale newer-server writes.

`player.cc::amem_play` bounds native PCM at four seconds; `amem_drain` is bounded and
responds to stopping. `audio_loop` schedules existing grains against VLC's clock.
This source review does not establish actual native underrun counts or close ABI.
GPU, audio-port and decoder failures retain uncertain resources; lifecycle-injection
suites test retry ordering, callback ownership and avoiding double disposal. Intentional
failure-state retention is not diagnosed as a leak from host RSS alone, and no unsafe
cleanup is added to make a soak graph flatter.

## Readiness boundary and next work

The core implemented scope has host evidence and must also obtain matching native
build evidence. Production readiness remains blocked by real account/PMS permissions,
streaming-session semantics, DVR consumer acceptance and PS5 presentation/audio/decoder/
firmware acceptance. Original reboot attribution, audio-close ABI, native resource
contracts and hardware 4K remain unresolved. These are external acceptance blockers,
not invented new source defects. Hardware hold and recording/tuning gates stay active.

Next independently testable work: richer PCM markers/output-model diagnostics and a
longer configurable soak, including additional sample-rate/channel layouts. True
native gaplessness requires measured output continuity and verified ownership contracts;
unsupported music conversion requires a verified Plex lifecycle before source enablement.

## Mixed-format candidate extension

See [MIXED-AUDIO-ACCEPTANCE.md](MIXED-AUDIO-ACCEPTANCE.md) for the independently decoded rate/layout matrix, calibrated timing, resource/partial-result reporting and extended-soak evidence. Native format boundaries now discard only obsolete copied PCM while preserving pause, and HTTP-200 library permission reductions fail closed. Production raster coverage is 40 frames. These are source corrections with host tests; matching native compilation remains an exact-head CI condition. [Future hardware plan](HARDWARE-ACCEPTANCE-PLAN.md) retains NO-GO and all external acceptance blockers.
