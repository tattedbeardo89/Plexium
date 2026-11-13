# Full-client implementation audit

This is a source audit, not a feature-complete release or PS5 acceptance record.
Physical hardware is unavailable; `ci/HARDWARE-HOLD` remains. The current milestone
connects playback negotiation and progress retry to the real session worker and
adds a real host software-decoding adapter. Protocol fixtures are synthetic,
not captured responses from an authorized live Plex server.

| Component | Classification | Actual source evidence and remaining work |
|---|---|---|
| Account authentication | Implemented, host-tested; live/native runtime unverified | `plex/auth.h`, `client.cc`, `app/session.cc`: device linking, bounded polling, primary-token persistence and restored-account validation. No authorized live account used. |
| Plex Home | Implemented, host-tested; native runtime unverified | `plex/home.h`, `Session::select_profile`: fresh profile permissions, protected PIN selection, avatars and context clearing. Managed tokens do not bypass profile selection. |
| Server discovery/reconnection | Implemented, host-tested; native runtime unverified | `Client::resources/select/identity`, `Session::connect`: authorized resources, identity/libraries verification, remembered routes and server preference. No HTTPS downgrade. Background automatic reconnect remains partial. |
| MachineIdentifier | Implemented, host-tested | Verified against discovery identity before accepting server; private per-profile server preference is revalidated, not trusted as authorization. |
| Movies | Partially implemented | Real Home/section hubs, paging, metadata/poster/backdrop/cast/related/collections/detail and Play/Resume. New version and quality controls. More filtering/sorting and UX remain. |
| Television | Partially implemented | Show/season/episode child browsing, series posters and watched metadata. Next-episode queue/autoplay is not connected. |
| Music | Partially implemented | Artist/album/track browsing and single-track Direct Play. Queue/repeat/shuffle helpers do not establish a complete music player UI. |
| Continue Watching/Home | Implemented, host-tested; native runtime unverified | `home_feed.h`, `Session::home/remember_position`: progressive personalized rows and scoped warm cache, immediate resume updates. Server-authored completion is refreshed through Home refresh. |
| Artwork | Implemented, host-tested; native runtime unverified | Scoped bounded disk/decode cache, three owned workers, profile/context generations and fenced native upload ownership. Console GPU acceptance unrun. |
| Native media player | Implemented but native runtime unverified | `player.cc`, `mpvshader.cc`, `ps5vdec.c`; lifecycle fault tests exercise actual source with controlled external boundaries. |
| Direct Play | Implemented, host-tested; native runtime unverified | Session dispatch uses selected original single-part H.264/HEVC MP4/MKV when capability policy accepts it. Conservative 1080p policy; native HEVC acceptance unrun. |
| Direct Stream | Implemented, fixture host-tested; live/native runtime unverified | Session now consumes universal decision, validates output and opens matching HLS start request. No live PMS remux streamed. |
| Transcoding | Implemented, fixture host-tested; live/native runtime unverified | Decision/start URL, profile capability query, session identifier, keepalive and stop; audio transcode is correctly classified even with copied video. Immediate Direct Play open failure retries once through negotiation. No live PMS transcoder exercised. |
| Playback state | Implemented, host-tested; native runtime unverified | Session exposes Idle/Resolving/Opening/Buffering/Playing/Paused/Seeking/Stopping/Stopped/Error/Released. Opening does not imply decoding. Actual host VLC port supplies observations; native port reads production player state. |
| Watch-state synchronization | Partially implemented | Profile/server-scoped bounded in-memory outbox, latest-update coalescing, backoff, at most two retry sends per dispatch, playing/pause/seek/stop timeline calls. No durable offline journal, exactly-once guarantee, or explicit watched/unwatched UI. Server owns watched thresholds. |
| Audio selection | Partially implemented | Existing native VLC track cycling and platform stereo/surround adapter; codec policy participates in negotiation. Named Plex-stream selection and preference persistence are not connected. |
| Subtitles | Partially implemented | Existing native VLC track cycling, default off, external Plex URL validation helpers. Server negotiation explicitly requests none and rejects burn decisions. Persisted named selections and subtitle-driven fallback remain work. |
| Search | Implemented, host-tested; incomplete product UX | Bounded worker-side section search, results and controller entry. Grouped filters/debounced incremental search remain partial. |
| Metadata caching | Implemented, host-tested | Bounded in-memory detail/page/Home caches, clear on authorization context switch. No replacement metadata database. |
| Personal Live TV discovery | Implemented, host-tested; live/native runtime unverified | `live_tv.h`, `guide.h`, Session guide paths use authorized PMS DVR/provider/lineup/grid contracts. No direct ErsatzTV backend. |
| Native guide | Implemented, host-rendered; native runtime unverified | Actual `View::guide_frame`: channel number/name/logo, timeline, current/upcoming airings, descriptions, progress, Now, program details, controller navigation, paging, clipping, loading/error/stale states. Existing deterministic guide tests and native draw-list screenshots remain. |
| Live TV session/tuning | Interface only; production disabled | `live_tune` deliberately refuses acquisition. Consumer ownership, heartbeat, tuner release and channel switching need verified real DVR contracts. No fixture playback is represented as DVR acceptance. |
| Favorites/last channel | Missing from personal guide UI | No claim of implemented tuning-dependent behavior. |
| Recording interface | Partially implemented protocol helper only | Read/cancel rule helpers in `live_tv.h` are not a completed recording UI. No new DVR infrastructure. |
| 4K decoder management | Implemented low-level substrate, runtime unverified; safe eviction blocked | 8/10-bit NV12/P010 substrate, bounded frame setup and checked cleanup/quarantine. Shared queue/CPU arena cache release contract unknown. 4K quality is not selectable; requests are capped to 1080p. |
| Full-client native build | Configured, result recorded in PR/CI | Separate matrix mode `PLEX_STARTUP_ONLY=0`, held package/manifest/debug/source/checksums. Stage 1 remains separate and initializes no session/player. Successful compile cannot prove runtime. |

## Connected playback behavior

Details expose L1 media-version cycling and R1 quality selection. Original or
local Automatic prefers compatible Direct Play. Other requests negotiate a
single-part HLS stream with PMS, using the same session and query for decision
and startup. Video output must be H.264/HEVC and no larger than 1080p when size is
reported; audio must match the compiled-engine policy. Unsupported/malformed
responses, subtitle burning and multipart output are rejected. This is an
implementation policy, not a PS5 hardware capability certificate.

A failed immediate original open receives one negotiated retry. An early native
end/error before playback observation also receives that fallback. Mid-playback
errors are surfaced for explicit stop/retry rather than causing an unbounded
fallback loop. Successful HTTP and accepted asynchronous open remain distinct
from player observations and decoded-frame evidence.

Streaming release failure retains the session and its original authorization
context. A new open cannot overwrite that ownership; Stop retries release.
Profile switches discard progress updates from the old context. In-flight server
work is bounded by transport timeouts, but no hard total shutdown time is claimed.
On worker shutdown a final bounded release is attempted. A failed final release
can still leave PMS work until server expiration; this is not reliable live
server cleanup acceptance.

The progress outbox holds at most 128 media updates and retries latest state;
it is not durable across process exit/profile changes. It never replays the old
profile's updates under the next profile's token. Timeline completion/watched
semantics remain server-owned; no guessed completion percentage is scrobbled.

## Reusable host integration

`tests/fixtures/plex_server.h` supplies account/profile/server/metadata/decision/
timeline/session fixtures to the production Session tests and real VLC test.
The latter explicitly maps a synthetic part URL to generated local media. This
mapping does not test PMS HTTP delivery or HLS/remux/transcode media output.
`tools/host-vlc.h` implements the same PlaybackPort with system libVLC software
video decoding and silent decoded-audio callbacks. It is reusable by a future
host application runner; the full interactive native player/renderer is not
ported. Existing `plexium-render` runs the actual native View with simulated
controller input and software rasterization. It is not a separate web interface.

Run `bash host/check.sh` (all original eleven suites plus playback integration),
`SANITIZE=1 bash host/check.sh`, and `bash host/test-vlc-integration.sh` (requires
libvlc development/runtime/plugins and FFmpeg). `SANITIZE=1` instruments the host
adapter and shared application, not the distro's prebuilt VLC/codec libraries.
Existing native-entry, Vulkan, decoder/player/audio, NDEBUG and HTTP/TLS tests
remain mandatory. Baseline tests require no account, external PMS or media.

## Local validation for this milestone

- Original eleven host suites plus new production-session playback integration:
  normal and ASan/UBSan runs. Added deterministic session tests cover all three
  playback paths, version selection, malformed decisions, opening/buffering/
  playing observations, pause/seek timeline, retry backoff, keepalive, failed
  release retention/retry, immediate fallback and sign-out isolation.
- Real system VLC software decoding: three generated 1920×1080 H.264/AAC sessions,
  decoded frames/audio samples, pause/resume, backward seek position, stop with
  no further frame/audio callbacks, reopen; normal and ASan/UBSan. No audible
  hardware output or PS5 decode is implied.
- Initial real-VLC LSan failure (396 bytes in the extracted runtime) was traced
  through plugin loading to Fontconfig and OpenMP initialization. The single-player
  host adapter explicitly initializes/finalizes Fontconfig and links/cleans up
  OpenMP after VLC releases. Final tests retain leak detection; no suppression,
  leak-disable flag, fake-success condition or plugin-disable workaround is used.
- Production Vulkan/font/texture ownership injection and actual player/audio/
  decoder cleanup injection pass normally and under ASan/UBSan. Existing
  native-entry injection normal/sanitized, NDEBUG filesystem/session checks,
  both startup-mode host syntax, ten curl and ten GnuTLS HTTP/TLS cases pass.
- Seven real View raster frames remain, including guide and updated details;
  detail L1/R1 actions are now asserted against the actual View. Packaging tests
  verify Stage 1, mode mismatch rejection, explicitly held full-client packaging
  and post-validation archive mutation rejection.

Exact-head native CI outcomes and artifact links are recorded in draft PR #2.
The native workflow builds both modes separately, preserving original toolchain
pins, package/signature checks, third-party notices and the hardware hold.
