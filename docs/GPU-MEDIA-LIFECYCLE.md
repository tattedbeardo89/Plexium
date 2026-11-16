# GPU, video and audio lifecycle milestone

Baseline: `5deff5b267a7bb07acd30b9c00b8f88d15dcfb94`; its verified native
[run](https://github.com/tattedbeardo89/Plexium/actions/runs/43454177788) and
held artifacts remain unchanged. That success did not prove PS5 launch safety.
The original console reboot still has no fault PC, matching crash context or
kernel/display trace identifying a cause. The previous startup investigation,
native comparisons, source pins and hardware hold are preserved.

## Resource ownership

The table describes source behavior. Mocked boundaries do not establish native
driver, audio or decoder behavior; exact-commit native compilation is separate.

| Resource | Allocator | Owner | Thread | Consumers | Lifetime | Safe destruction requirement | Timeout/error | Status |
|---|---|---|---|---|---|---|---|---|
| Session state/token/socket/TLS request | Session/native transport | Runtime, inside Application | 512 KiB session worker | UI snapshots, Plex API | Account/server epoch | Close command queue, invalidate epoch, finish current request, join | Bounded close; retain entire Application if worker remains | Implemented; host fixtures |
| Artwork download/decoded pixels/cache job | Artwork/Task | Three 1 MiB lanes | Artwork workers | Render-thread upload | Profile generation/context | Cancel at request/decode boundaries; finish and join | Two-second aggregate close; retain owner on timeout | Implemented; production artwork tests |
| View callbacks | Application | Application | Render thread | View | Current Application | Clear callbacks before stopping workers | No renderer callbacks after close | Implemented; source/native syntax |
| Artwork staging buffer/memory | gfx_buffer | Native texture manager | Render thread | One upload command | Upload fence | Successful fenced completion before unmap/free | Failed submit/wait retains staging and image; disables uploads | Implemented; production texture tests |
| Artwork image/view/descriptor/sampler | gfx_image/ImGui descriptor pool | Native texture manager | Render thread | Prior/current UI frames | Render context; CPU request tagged by profile generation | Fence drain before disposal, no new uploads after closing | Failed completion retains allocations | Implemented; production texture tests |
| Font image/view/descriptor | ImGui Vulkan backend | ImTextureData/backend context | Render thread | ImGui frames | Backend context | Partial allocation ownership immediately attached; drain before backend destruction | Pre-submit errors can clean after drain; uncertain GPU use retains context | Implemented; real backend injection |
| Font staging/command/fence | ImGui Vulkan backend | Backend context | Render thread | Font upload | One synchronous upload batch | Five-second fence success before staging/fence disposal or command-pool reset | Submit failure/timeout retains all handles, blocks retries/draws | Implemented; 16 operations plus timeout tested |
| UI vertex/index buffers | ImGui Vulkan backend | Backend frame ring | Render thread | UI frame commands | Frame ring slot | Slot completion before resize; check allocation/map/flush before drawing | Failed upload aborts draw; owned partial objects remain for cleanup | Implemented; source-reviewed/native compilation required |
| Frame/one-shot/queue-drain fences | Vulkan | gfx | Render thread | Renderer and uploads | Submitted batch | Bounded fence completion | One uncertain batch closes queue; no unsafe retry/free | Implemented; real gfx injection |
| Swapchain/images/presentation semaphores/device | WSI/Vulkan | gfx/driver | Render thread/display engine | Display engine | Render context/process | Presentation completion proof, beyond a graphics fence | Retain WSI/device if proof unavailable; return failure | BLOCKED on verified PS5 present completion |
| VLC instance/player/current/queued media | libVLC | Native player globals, owned release job | Render owner/release worker | VLC callbacks, stats | Player/media epoch | Join stats/audio/auxiliary jobs, then stop and release in owned job | Two-second waits; timed-out job and context stay owned | Implemented; production player with controlled VLC boundary |
| CPU fallback picture | vmem_setup checked allocation | Native player | VLC callbacks | Discarded frames | Player/format | Successful setup before use, serialized fallback use, quiesced callbacks before free | Invalid/oversized format or allocation fails setup; closing rejects new allocation | Implemented; actual callback fixture |
| Video callback buffers/garbage/planes | vmem/Vulkan | Native player | VLC callback/render threads | Decoder, copy workers, GPU | Format/media generation | Stop/release VLC first; join copy pool; successful GPU drain | No video destruction if either callback or GPU completion is uncertain | Implemented; production disposal fixture |
| Video/upscale/MPV images/pipelines/pools/queries | Vulkan | Native player / MPV | Render thread | Native video draws | Player/render context | Media released and graphics fence drained | Retain on failed prerequisite | Implemented; host disposal fixture; native behavior unverified |
| Temporary video shader modules | gfx_shader | Bounded local shader owner | Render thread | Pipeline creation | Initialization call | Destroy on every return; reject missing shader modules | Failed allocation cannot feed a null shader to pipeline creation | Implemented; source and compilation |
| Decoder handle/AU/frame/CPU/GPU memory | Videodec2 plugin | decoder_sys_t | VLC decoder thread | Native decoder/GPU | Decoder instance | Successful DeleteDecoder before memory release | Failed delete/unmap/backing release retains ownership; retry does not repeat completed delete/unmap; failure latch blocks new playback and allocation | Implemented; actual C teardown injection |
| Shared decoder compute queues/CPU arena | Videodec2 plugin | Process-lifetime plugin cache | Decoder/runtime | Subsequent decoder instances | Process | Verified queue/arena release contract | No speculative release/idle eviction | UNVERIFIED / retained; 4K work deferred |
| Row-copy workers/job buffer | Native player | CopyPool | Up to three explicit 512 KiB workers | Decoder plane conversion | Player | Finish finite copy, signal close, join, then destroy pool | Partial thread creation falls back to fewer workers; never detach | Implemented; 32 production copy/release cycles |
| Subtitle/search/screenshot jobs | Native player | Eight-job bounded owner | Explicit 1 MiB workers | Media/subtitle state | Player/media generation | Close admission, invalidate media serials, join before VLC release | Retain owner if deadline expires | Implemented; owner-capacity/cancellation tests |
| Audio queue/interleaved blocks | amem/native player | Native player | VLC callbacks/audio worker | Audio output | Current media/player | Stop callbacks and worker, clear queue | Timed-out worker prevents port/context release | Implemented; host lifecycle fixtures |
| Stereo/surround ports | platform audio APIs | PlexAudioPorts | Init/audio callback, then quiesced shutdown | Audio worker | Player; reused between media | Audio and VLC callbacks joined before close | Missing/failed close retains handle; no double close/reopen leak | Implemented; helper and player boundary tests; PS5 API unverified |

## Completion and failure behavior

Reachable application GPU waits now use fences with a five-second timeout.
Artwork one-shot submissions retain command/fence/staging/image ownership on
uncertain submission or completion. Font upload uses a backend-owned fence and
staging fields; it does not reset/retry a command pool after uncertainty. Failed
image, descriptor, memory, bind, map, flush, recording or submission operations
return before invalid memory or descriptors can be used. CPU-side profile
generation checks reject stale artwork before upload; native texture handles
also record the owning render-context generation. Closing rejects new uploads.

A graphics fence **does not prove display-engine presentation completion**.
After successful graphics drain, renderer-owned children can be freed; WSI
images, presentation semaphores, device, surface and instance remain owned when
presentation completion cannot be established. `gfx_destroy` returns false in
that case. It does not silently translate retained ownership into success.
Only an explicitly proven presentation-complete caller may release those
remaining dependencies; current native application code provides no such proof.

The vendored backend's unused multi-window/resize helpers still contain idle
waits; Plexium does not expose or call those routes. Do not enable them without
the same lifetime work. Public renderer/full-client paths no longer call
device-wide or queue-wide idle waits.

## Player and audio lifecycle

Source exposes Uninitialized, Initializing, Ready, Opening, Playing, Stopping,
Stopped, Released, Error and Recovery states. Initialization failures enter
Error; recovery retries cleanup rather than reopening an uncertain decoder.
Video format setup bounds dimensions/frame bytes and allocates its owned fallback
before decoding; the lock callback no longer reallocates/leaks scratch storage.
An early shutdown request rejects new format/audio allocations while workers
finish. Stop clears queued media/audio and invalidates subtitle generations. Release
includes the previously omitted queued media reference. A completed release is
idempotent; failed audio close retries without double-releasing VLC objects.

Audio/stats/stop/release and auxiliary work use explicit-stack owned workers.
The row-copy pool is no longer leaked with detached threads. It signals idle
workers, joins them after VLC callback quiescence, and can be initialized again.
Temporary video shader modules now have bounded local ownership. After media
release and a graphics drain, final video teardown releases slots, deferred
buffers, plane images, render targets, pipelines, layouts, descriptor pools,
samplers, timestamp queries and MPV plans. None of these releases run on a stop
or GPU-completion timeout.

`sceAudioOutClose` is a weak runtime boundary: absence or a negative result is
reported as failure while the port remains owned. This is not a claim that the
API succeeds on firmware 13.40. Stereo/surround ports are reused between media
and closed only after their output worker and callbacks finish.

## Shutdown coordinator and diagnostics

Worker steps 16–19 clear UI callbacks, close render/upload admission, invalidate
and join the session, and cancel/join artwork. Each session/artwork close has a
two-second wait. If either cannot finish, the Application deleter retains the
whole owning object; it does not delete state referenced by its worker. Direct
stack owners still join on destruction as a safety fallback; timed-out owners
must be retained by callers that need bounded shutdown.

Render/media steps 0–6 reject uploads, release media/callbacks/audio, drain GPU
work, dispose video objects, dispose artwork/font/backend objects, release
renderer/WSI ownership where proven, and destroy the UI context where safe.
Each allocated step reports started, completed, failed or timed-out; absent or
blocked dependencies report skipped. Duplicate steps do not execute twice.
Unresolved ownership returns exit status 4 instead of logging a clean shutdown.
The native exit bridge preserves that status if LoadExec returns, and avoids
global stdio flushing while retained workers may hold stream locks.

Startup logs include the real source SHA/source date and Stage 1/full-client
mode. Crash diagnostics include lock-free last startup/shutdown phase numbers;
phase labels and numeric error codes contain no Plex paths, tokens or PINs.
Logs are bounded by fixed phase counts, not per-pixel/per-callback tracing.

## Regression evidence and limits

The 11-suite ordinary/ASan/UBSan baseline remains. New production-boundary tests
exercise font upload failures across 16 operations plus fence timeout; closed
and failed upload gates; artwork staging timeout retention and repeated render
contexts; WSI retention; owned job capacity/timeouts/retry; shutdown idempotence;
audio port close failure/retry; actual decoder delete/unmap/backing-release failure-before-free;
partial video initialization; 32 real player/copy-pool release cycles; blocked
VLC stop retention/retry; queued media cleanup; actual video object disposal;
session/artwork close during blocked network requests. Synchronization uses
events/promises and explicit deadlines rather than timing-based mock success.

VLC/platform/driver calls are controlled boundaries in host tests. These tests
do not decode media or play audio. Native CI must compile this exact source,
including the decoder module; host headers and native toolchain pins are
separate. New host CI installs official VLC development headers for the actual
player/C decoder teardown tests. This does not change native dependency pins.

Remaining blocking/retention limits:

- Driver entry points, audio output/close, DNS/filesystem operations and OS thread
  termination cannot be assigned a hard wall-clock guarantee by a fence timeout.
- VLC 3 stop/release and network parsing can block. They are isolated in owned
  jobs with timed waits; their objects remain alive on timeout. Initialization
  (`libvlc_new`/player creation) still executes on the render owner and is not
  asynchronously interruptible in this milestone.
- Presentation completion is unverified. Native Stage 1 can now intentionally
  report incomplete WSI cleanup (status 4) on exit; that is an ownership hold,
  not evidence that the original reboot recurred.
- Process-lifetime decoder compute queues/CPU arena have no newly verified
  destruction/4K eviction contract. No speculative API calls were added.
- Native audio/decoder ABI behavior, physical picture/audio/seek/resume,
  long-duration operation and firmware/jailbreak compatibility remain untested.

**Physical Stage 1: NO-GO.** Preserve `ci/HARDWARE-HOLD`, existing artifacts,
working installation and `/data/Plexium/`. No transfer, install, launch, merge or
stable release is authorized by this milestone.

Direct Play dispatch, Plex Home, scoped browsing/artwork and the personal Plex
cable guide are preserved. Direct Stream/transcode/session negotiation is not
newly enabled; Live TV tuning remains disabled pending verified authorized
acquisition/heartbeat/release. There is no direct ErsatzTV access or duplicate
backend. Next: establish PS5 presentation completion and native audio/decoder
release contracts, then validate a reliable 1080p Direct Play lifecycle before
4K memory eviction or Live TV playback.

Timeout values are failure budgets, never evidence of completion: only the
recorded worker/fence state permits destruction.

## Local validation record

Implementation commit: `fc7ed0d82ad8680513f2bade659b84faf05a1e9b`.
The subsequent report commit changes documentation only; native artifacts must
still identify their own exact workflow HEAD. Local checks passed:

- `bash host/check.sh` and `SANITIZE=1 bash host/check.sh`: retained 11 suites,
  HTTP/TLS, shared frames, branding, diagnostics/finalizer, imports/private logs.
- Production gfx/backend/texture ownership injection normally and sanitized:
  initialization, 16 font operations plus timeout, nine vertex/index operations,
  one uncertain GPU batch, WSI retention and repeated texture contexts.
- Production player and C decoder teardown injection normally and sanitized:
  32 copy/player cycles, fallback format/closing bounds, stop and audio-output
  timeouts, failed port close, invalid audio input, queued media and decoder
  delete/unmap/backing-release errors with safe retry.
- Actual Stage 1 entry injection normally and sanitized; both startup modes
  under host syntax with exceptions/RTTI disabled; ten production GnuTLS/socket
  HTTP/TLS scenarios; NDEBUG filesystem/session checks.

The new draw-list fixture initially destroyed its context too early. Its cleanup
order was corrected; the production assertion remains enabled. Official Ubuntu
VLC 3.0.24 development headers were extracted locally for player/C decoder tests
because this environment lacks a privileged package installer. Native toolchain
pins were not changed. Host builds do not substitute for native compilation.

## Exact changed files

```text
.github/workflows/host.yml
.github/workflows/native.yml
THIRD_PARTY.md
docs/ARCHITECTURE.md
docs/GPU-MEDIA-LIFECYCLE.md
docs/NATIVE-REBUILD.md
docs/STAGE1-INSTALLATION.md
host/test-gfx-failures.sh
host/test-player-lifecycle.sh
ps5/build-title.sh
ps5/modules/ps5vdec.c
src/app/session.cc
src/app/session.h
src/audio_ports.h
src/gfx.cc
src/gfx.h
src/library.h
src/main.cc
src/mpvshader.cc
src/mpvshader.h
src/platform.h
src/platform_ps5.c
src/player.cc
src/player.h
src/plex/artwork.cc
src/plex/artwork.h
src/runtime/owned_jobs.h
src/runtime/shutdown.h
src/textures.cc
tests/artwork_tests.cc
tests/decoder_release_tests.c
tests/gfx_failure_tests.cc
tests/gfx_ownership_tests.cc
tests/native_entry_tests.cc
tests/player_lifecycle_tests.cc
tests/runtime_tests.cc
tests/startup_runtime_tests.cc
tests/texture_lifecycle_tests.cc
tests/ui_backend_failure_tests.cc
third_party/imgui/backends/imgui_impl_vulkan.cpp
third_party/imgui/backends/imgui_impl_vulkan.h
```
