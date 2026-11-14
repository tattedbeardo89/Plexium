# Startup lifecycle review — 2026-11-13

## Native CI inspected

[Run 43450739707](https://github.com/tattedbeardo89/Plexium/actions/runs/43450739707)
completed successfully for `c8d597e6b37acbbf7273aa5f2b8093e9b2dadca2`.
The complete native-job log was retrieved and inspected. Host regressions,
ASan/UBSan, pinned dependency-cache restore, all 22 source digests, actual PS5
compile/link/sign, SELF validation, matching source/debug identity and package
CRC/modes passed. The hardware-hold script skipped draft-release replacement.
Archive-mirror 404s were recovered by the existing fallback; they did not fail
the build or change pins. No physical execution occurred.

`git diff c8d597e c23ef02` contains only `docs/ARCHITECTURE.md` and
`docs/ARTWORK-GUIDE-HARDENING.md` (77 additions). Runtime, build, tests, assets,
toolchain inputs and vendored code are identical. **Artifacts are c8d597e builds,
not c23ef02 builds**, despite that exact code equivalence.

The delivery artifact was independently downloaded. Its outer SHA-256 matches
GitHub metadata (`a6368f02ecc879ae0200fb09659bfe83ce21b0b5f08797179f648c489baa6406`);
ZIP CRC, its four checksummed inner documents, and consistent manifest/validation
identities passed. Binary artifacts exceed the workspace's 32 MiB transfer limit;
their payload verification is established by native CI, not a second local
binary download. A signed-URL download also returned HTTP 403; no network or
verification restriction was bypassed.

| c8d597e artifact | Identity / location |
|---|---|
| [Installation](https://github.com/tattedbeardo89/Plexium/actions/runs/43450739707/artifacts/14052635204) | Inner `Plexium-0.5.0-stage1.c8d597e6b37a.zip`; SHA-256 `e8a13eec4a2ba3ef896054d595c76c09e5a194bc6cdc78f0d5081070a73f3157` |
| [Debug ELF](https://github.com/tattedbeardo89/Plexium/actions/runs/43450739707/artifacts/14052535183) | SHA-256 `a9736de32a9b4db585968db7a5d53922372ebee62ffda1b10d61b7c97fb14b58`; build ID `7aa8d7f28b43bd00d44670b64de76b49068e39c2` |
| [Manifest/checksums/instructions](https://github.com/tattedbeardo89/Plexium/actions/runs/43450739707/artifacts/14053215028) | PPSA85421, startup_only=true, held, hardware_tested=false |

The immediate main/bootstrap reservation was 424 bytes, excluding saved registers
and all callees. That number is not a total stack bound. The artifacts above
predate the new lifecycle corrections below and must not be relabeled.

## Original fault: evidence and attribution

The available evidence remains the user's reboot/recovery report, the retained
PR #1 investigation and historical executable comparisons in `STARTUP-AUDIT.md`
and `NATIVE-COMPARISON.md`. There is no supplied console fault PC, matching native
crash context, launch-time Plexium log or kernel/module trace establishing the
failing instruction. The published 0.1/0.2 comparisons found matching constructor
order/import sets, valid SELF structure and consistent type layouts; stack
growth was measured and reduced. These do not establish the reboot cause.

Signal 6 is consistent with an abort/assertion but does not identify its source.
Malformed JSON is covered by defensive parser fixtures; it is not proven to have
caused this launch failure. Do not infer a hardware fix from host checks.

Current startup order: platform/private diagnostic setup and signal handlers;
loader/instance/GPU discovery; display surface; logical device; swapchain;
render targets and optional HDR targets; ImGui context/font/backend; controller
frames; splash dismissal after frame three. Stage 1 constructs no Plex session,
artwork worker, VLC player or decoder. A failure before visible UI can still
occur during hidden rendering/font upload.

## Concrete corrections in this checkpoint

1. The real ImGui Vulkan backend previously ignored initialization allocation
   errors, used uninitialized descriptor/pipeline output locals on failure, and
   reported successful device-object creation. It now stops at each failed
   allocation, cleans partial ownership, handles a partially created pipeline,
   detaches its backend and returns false without the unconditional init assert.
   A bounded first-error callback logs one Vulkan result code. This is a proven
   failure-injection defect, not a proven console reboot cause.
2. Surface/GPU enumeration results are checked before reading them. Counts,
   dimensions and capabilities are bounded. Failed capability queries cannot
   feed uninitialized stack data to swapchain creation. The fallback composite
   alpha mode chooses one supported bit rather than an invalid multi-bit mask.
3. Successful Stage 1 shutdown destroys renderer-owned resources after device
   idle and texture/backend destruction. Partial init is supported; WSI-owned
   images are excluded; HDR ownership survives optional-init failures; cleanup
   resets handles and is idempotent. Failed GPU completion retains uncertain
   resources until process exit.
4. Full-client shutdown explicitly closes render requests, invalidates/joins
   the session worker, clears its public authorization snapshot, then closes
   artwork before UI/player teardown. Artwork cancellation is cooperative at
   request/decode boundaries; shutdown never schedules maintenance/uploads.
   Texture allocations remain owned by the native texture manager until idle.
5. The first controller poll previously put a 64 by 120-byte sample array
   (7,680 bytes) on the entry thread's stack, beyond the reported main/bootstrap
   frame alone. It now uses one bounded render-thread-owned static poll buffer;
   the existing pad backend already has single-thread static ownership. Native
   stack reporting now measures `plat_pad_read` and the entry/controller pair,
   with the same 5,224-byte immediate startup budget. This is another concrete
   stack reduction, not proof that the original native stack overflowed.

The MIT ImGui backend modification is local; its license and upstream attribution
remain. No reference implementation or alternate media backend was imported.

## Ownership and remaining limits

| Owner | Cleanup / limit |
|---|---|
| Session worker | Explicit 512 KiB stack, closed command queue, generation invalidation, join before owner destruction; current network operation completes under adapter timeouts |
| Artwork lanes | Three 1 MiB stacks, cooperative close, joined callbacks/results; no GPU calls from workers |
| NativePlayer queue | Closed before session join; pending calls wake/reject instead of waiting for a dead render thread |
| Stage 1 Vulkan objects | Backend/texture children, then renderer children, device, surface and instance after successful idle |
| Failed GPU completion | Retained until process teardown; destruction is not attempted on uncertain resources |
| Full-client VLC/video/audio | Player/audio threads stop before UI teardown, but all video/shader/decoder allocations are not yet explicitly freed; full-client device destruction is deliberately deferred |
| Settings/profile scope | Existing private atomic records/migration and profile-generation clearing retained; no state deletion or hardware access |

**Shutdown is not completely bounded.** `vkDeviceWaitIdle`, ImGui texture-upload
`vkQueueWaitIdle`, VLC stop/release and driver/audio operations may block. A worker
join waits for its current operation; it is not forcibly detached or destroyed
on timeout. Adding an unsafe timed detach would create use-after-free risk.
Backend runtime font/texture-upload failure handling remains separate from the
new allocation-init tests. Full-client decoder eviction/4K restoration, audio
port closure, Vulkan video-object teardown and signal-handler registration error
handling need further work. No deadline or complete resource-reclamation claim
is made for these paths.

## Validation and release gate

Required checks are the retained 11-suite ordinary/ASan/UBSan host workflow,
both native-entry modes under syntax, actual Stage 1 entry injection, production
Vulkan frame/surface-query failures, real backend allocation injection, renderer
partial-ownership teardown, artwork close/profile disposal, session close and
both HTTP/TLS adapters. New backend tests cover all 13 initialization allocations;
ownership tests cover both WSI/offscreen and partial resource sets. These are
Linux tests, not PS5 ABI or physical compatibility evidence.

Local ordinary and ASan/UBSan full workflows passed after the final session-close
changes. Stage 1 entry injection passed normally and sanitized; production
Vulkan/backend/ownership tests passed normally and sanitized; the production
GnuTLS/socket adapter passed ten HTTP/TLS scenarios. No failing assertion or
compiler warning was suppressed. Native-source syntax is checked separately
and does not stand in for the required native CI compilation.

The fresh push must compile/sign/package this checkpoint through pinned native
CI and publish a distinct versioned Stage 1 ZIP, matching debug ELF, manifest,
checksums and validation report. Earlier artifacts remain unchanged. Inspect
that exact run and identity separately from c8d597e.

**Physical Stage 1 recommendation: NO-GO.** Original fault attribution and native
driver/hidden-frame behavior remain unresolved. Do not install, transfer, launch
or ask for another reproduction. Only after independent compatibility/fault
review and separate authorization could a manual test be limited to one startup,
controller/frame observation and Options + Circle exit, preserving the working
installation and `/data/Plexium/`. See `STAGE1-INSTALLATION.md`.

Next milestone: bounded font/texture submission/completion and complete native
video/audio teardown, followed by verified Plex playback negotiation/session
cleanup. Personal Plex DVR discovery/cable guide remain; tuning stays disabled
until authorized acquisition/heartbeat/release is verified. No direct ErsatzTV
access, recording backend or schedule generation was added.
