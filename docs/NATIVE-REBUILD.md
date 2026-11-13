# Native rebuild checkpoint — 2026-11-12

Branch: `feature/plexium-native-rebuild`. Base: PR #1 head
`d2b5e7cc01aa4162375e938e6ec1e0e9beeb96fe`; `main` is an empty review baseline.
This checkpoint retains history and does not merge or modify PR #1.

## Audit and retained work

Retain the Plex account/device-link, scoped Home/PIN, machineIdentifier discovery,
permission-checked libraries, asynchronous session, artwork cache, protocol tests,
Vulkan/VLC playback substrate and pinned build/sign/package recipes. They are
substantial real implementation, but historical build records do not validate
this branch. Source licensing is GPL-3.0-or-later with upstream notices retained.

Supersede the always-full bootstrap: it eagerly loaded the decoder in platform
initialization, created the player before any visible frame, used an unspecified
session thread stack, returned without shutdown on failures, and had no reachable
normal exit. The existing hardware hold is retained. The prior reboot cause is
unresolved; these changes do not establish a reboot fix.

## Implemented P0 changes

- `PLEX_STARTUP_ONLY=1` is the native build default. Platform initialization skips
  decoder load/probe. The entry creates only platform diagnostics, renderer and
  ImGui, displays controller/frame status, and exits with a one-second
  Options + Circle hold. It constructs no Plex session, artwork worker or VLC player.
  Media dependencies remain linked; this is runtime isolation, not a minimal ELF.
- `PLEX_STARTUP_ONLY=0` explicitly selects the retained full application path.
  Neither mode is authorized for installation by this checkpoint.
- Entry scope tracks completed/attempted subsystems and reaches shutdown before
  platform exit on normal exit and initialization/frame failures. Failed graphics
  frames retain uncertain GPU allocations until process teardown.
- The session worker requests a 512 KiB pthread stack, checks creation errors and
  fails closed; accepted worker stack requests are bounded to 64 KiB–2 MiB.
  This does not change third-party VLC, artwork async or driver thread stacks.
- Session queue capacity is 16 and duplicate queued timeline ticks are rejected.
  The media queue has eight slots; shutdown rejects pending work and wakes callers
  before joining the session worker. Expired work cannot execute later.
- Startup logging no longer allocates a 256 KiB static buffer or detached flusher.
  A missing packaged UI font falls back to the built-in font before ImGui asserts.

## Tests and limits

`bash host/check.sh` includes worker bounds/restart/join and render-queue
saturation, cancellation, timeout and shutdown tests. `host/test-native-entry.sh`
executes the production Stage 1 entry with actual ImGui and injected platform/GPU
adapters: graphics/backend failure, frame acquire/submit failure, controller exit,
and successful context destruction; failed GPU frames retain the still-attached
backend context until process exit to avoid ImGui shutdown assertions. This does not execute Vulkan, PS5 imports or a console.
`host/check-native-source.sh` syntax-checks both startup modes with exceptions and
RTTI disabled. CI runs the new entry tests normally and under ASan/UBSan.

Remaining P0: native cross-build and matching stack/import/package measurements;
physical Stage 1 acceptance; complete GPU/player/platform allocation destruction;
bounded shutdown of third-party decoder and driver operations; explicit artwork
and media thread stack policies. Existing `gfx_shutdown` waits for device idle;
VLC shutdown may block. The entry's failure handling is not a guarantee of kernel
recovery. No PS5 was contacted.

## Hardware release gates

| Stage | Source implementation | Host evidence | Physical PS5 | Gate |
|---|---|---|---|---|
| 1 startup/render/controller/exit | New isolated path | Failure-injected actual entry; syntax | Unrun | Held: new native build and independent compatibility review required |
| 2 auth/Home/server | Retained full path | Existing fixtures and HTTP/TLS tests | Unrun for this branch | Held behind Stage 1 |
| 3 library/artwork | Retained full path | Shared view/protocol tests; native syntax | Unrun | Held behind Stage 2 |
| 4 1080p | Retained VLC dispatch | Dispatch fixtures only | Unrun | Held behind Stage 3 |
| 5 4K/HDR/memory | Partial inherited decoder | No new decoder execution | Unrun | Unverified; resource manager incomplete |
| 6 personal DVR/EPG | New bounded PMS adapter and native guide | Fixtures/shared native draw lists | Unrun | Held behind player stability |
| 7 personal Live TV playback | Unverified helper only | No live session acceptance | Unrun | Blocked: consumer/session lifecycle verification |
| 8 long duration | Incomplete | No endurance evidence | Unrun | Blocked by earlier stages |

The hardware hold does not request another console reproduction. Use existing
fault evidence and independent review first. No installable replacement or stable
release is recommended.

## Personal-server Live TV implementation

A permanent `Live TV` destination opens `Screen::Guide` through the session
worker, using the selected server/profile token. This is separate from the
retained experimental cloud Free TV destination. The personal adapter discovers
DVRs, matches authorized provider IDs, and fetches up to 200 channels through at
most four DVR providers. No-DVR and HTTP failure states remain separate. The
client never accesses tuner addresses or XMLTV URLs returned in DVR metadata.

Guide loading displays six channels and two hours per page, retains at most 100
programs per channel, filters time ranges, sorts numeric channel components and
programs, handles midnight by querying both UTC dates, and uses an eight-second
aggregate budget checked between bounded requests. A final in-flight request can
exceed that deadline by its 1.5-second timeout. No programming is generated when
Plex supplies none. Last-successful refresh time changes only after successful
page retrieval. Metadata/context clears when switching servers or profiles.

The actual native draw-list UI includes clipped timeline cells, channel names,
Plex-supplied logos, program descriptions, current-time marker, vertical paging,
horizontal time navigation, Jump to Now, refresh, empty states and 4K layout.
Guide fixtures remain in tests/tools and are never production content. Guide
playback, mini guide, favorites, recording actions and channel surfing are not
implemented. Selecting a channel reports that live session support is pending.

`live_tune` no longer posts a tune or fabricates an HLS URL. The official docs
confirm separate tuning/session/playlist APIs, but this task has no authorized
real server responses establishing consumer acquisition, heartbeat, release,
transcode negotiation or native MPEG-TS/HLS playback. These remain blockers.
The provider/channel/grid wire adapter is documented/source-reviewed and fixture
tested; it is not accepted against a real configured DVR.

## Durable state

The native account/client ID, profile-scoped server/route records, artwork,
preferences and diagnostic logs now use `/data/Plexium/`. Private directory
ownership and permissions are checked; logs reject symlinks and nonregular or
public files. Startup retains existing stderr if private storage is unavailable.
The first full-session restore copies recognized private credential/selection
records from `/data/plexium-client/`, leaving rollback data intact. It never
overwrites new records. A completion marker prevents sign-out from reimporting
old account tokens. Caches regenerate. Legacy title-folder `prefs.txt` is not
imported; legacy playback preferences must be set again. Token protection uses
0700 directories/0600 files, not encryption or a verified PS5 keystore.

Automatic restoration of the selected unprotected Home profile is still missing;
protected profiles continue to require PIN selection. Manual server entry/local
broadcast discovery, full quality/transcoding negotiation, subtitle styling,
queues, expanded settings and modern TV-client feature parity remain incomplete.

## This checkpoint's validation record

Code commit `f16b6a3b7238277921b1fabd93a4a8dd2501b972` passed:

- `bash host/check.sh`: nine C++ suites, ten curl HTTP/TLS cases, seven shared
  native rendered frames, focus/controller checks, branding, imports and log tests.
- `SANITIZE=1 bash host/check.sh`: same selected workflow under ASan/UBSan.
- Both startup modes with `PLEX_PS5`, no exceptions/RTTI: native source host syntax.
- Actual Stage 1 native entry failure injection, ordinary and ASan/UBSan.
- Production Vulkan frame/shutdown failure injection, ordinary and ASan/UBSan.
- Production GnuTLS/socket transport on Linux: ten HTTP/TLS scenarios.

Initial auxiliary invocations had missing header/object prerequisites; corrected
invocations above passed. No sanitizer failure was suppressed. Log digests are
in `rebuild-validation.json`. Host output is not a native ABI/compiler test.

Local `ci/build-native.sh` stopped before compilation because this environment
has no pinned PS5 Vulkan/SDK/media dependency installation. Docker is healthy
but has no prepared native image. Git push works despite the local gh CLI's
invalid API token; the GitHub connector can read PR metadata. The source push
started [native CI](https://github.com/tattedbeardo89/Plexium/actions/runs/43358805720)
and [host CI](https://github.com/tattedbeardo89/Plexium/actions/runs/43358805688).
Their results must be checked separately. No native CI completion or installable
artifact is claimed in this initial record.

Build packaging now records source SHA, title ID, source date and startup-only
mode in `build-manifest.json`. Native validation includes the new bootstrap
frame in stack accounting rather than measuring only its tiny main wrapper.
Source syntax and script checks do not establish that these native gates pass.

Recommended next step: complete the remote pinned native build and inspect its
stack/import/symbol/package evidence while preserving the hardware hold. Then
resolve the existing launch fault through independent review and existing
console evidence before considering a controlled Stage 1 hardware test. Do not
install the historical 0.2 candidate or this branch's output as a normal update.
