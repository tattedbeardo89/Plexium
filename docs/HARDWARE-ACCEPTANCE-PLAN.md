# Future hardware acceptance plan — offline preparation only

**NO-GO: this document does not authorize installation, launch, account linking or
physical Stage 1 testing.** `ci/HARDWARE-HOLD` remains. The historical reboot cause,
presentation completion and other ownership contracts are unresolved. Native CI,
host VLC and the extended soak do not resolve them. No console command is executed
or requested by this plan. Existing installation/rollback material remains intact.

## Evidence gate before any physical authorization

An independent review must address the existing [startup audit](STARTUP-AUDIT.md),
[GPU/media lifecycle](GPU-MEDIA-LIFECYCLE.md), [native comparison](NATIVE-COMPARISON.md)
and [Stage 1 hold](STAGE1-INSTALLATION.md). Required evidence includes reboot/failure
attribution, entry/import/stack/ABI review, display-engine presentation completion
and safe/incomplete shutdown behavior. A matching signed package and passing CI are
necessary provenance, not sufficient launch-safety evidence. The current answer
remains NO-GO; no countdown, automatic gate removal or runtime bypass exists.

Any later physical action requires separate explicit authorization after that
review. This milestone does not request it. Preserve original evidence and avoid
repeating the historical failure as an experiment.

## Offline candidate identity and symbol preparation

Keep Stage 1 and full-client packages separate. For each future candidate retain:
source SHA, mode, pinned toolchain/source inputs, installation payload SHA-256,
SELF/debug ELF hashes, ELF build ID, manifests, SHA256SUMS, licenses and CI URL.
Verify these offline using `sha256sum --check SHA256SUMS` and `readelf -n` on the
matching debug ELF. Inspect ZIP CRC, permissions and expected PPSA85421 root.
The outer Actions download ZIP hash differs from the installation payload hash.
Never symbolize a crash with another mode/commit's ELF. Preserve exact copies of
all historical packages and their identities; source kits are not installers.

Prepare a bounded evidence ledger with source/mode/build ID, stage, timestamps,
startup checkpoints, initialization result, thread/job ownership, exit result,
incomplete-cleanup flags and first failure. Preserve original logs/crash evidence
unchanged, plus redacted copies for review. Never record tokens, PIN entry, private
playback URLs or account secrets. Record symbolic crash addresses/registers only
through an authorized collection mechanism whose ABI is independently reviewed;
this plan invents no crash-capture syscall or remote console tooling.

## Conditional future sequence (blocked now)

| Stage | Scope after separate authorization | Evidence needed to advance |
| --- | --- | --- |
| Stage 1 startup | Startup-only manifest, platform/renderer/controller, existing Options+Circle exit; no Plex/decoder/audio initialization | Checkpoint sequence, actual presentation ownership/completion, controller input, matched symbols, clean exit or accurately reported retained ownership. Any failure/incomplete cleanup halts advancement. |
| Controller/UI | All directions, tap/chord resolution, focus, safe areas, protected-entry privacy, return/cancel and exit | No accidental destructive chord actions, clipped controls or stale profile scenes. Host screenshots alone are insufficient. |
| Audio initialization/close | Only after audio-close ABI and port/worker contracts are independently established | Stereo and verified surround configurations; format setup, pause/flush, stop, worker termination and close result. Retained ownership is a stop condition, not a pass. |
| Full-client account/library | Authorized test account/profile/server only after explicit permission | Correct identity, protected-profile PIN, server machineIdentifier, scoped artwork/cache/queue, sign-out and revocation clearing. No private credentials requested in this milestone. |
| 1080p playback | Controlled known-compatible media, then authorized negotiation/track/subtitle paths | Decoded audio/video, clocks, aspect/colour, pause/seek, bounded queue, stop/release, callback cessation and navigation/focus transitions. Native compilation is not playback evidence. |
| Extended playback | Only after shorter stop/release/resource tests pass | Repeated transitions, format changes, profile switching, reconnect, shutdown, resource/buffer measurements and error/quarantine behavior with matched symbols. |
| 4K | Blocked until 1080p, decoder/cache/arena ownership, memory/presentation and release prerequisites are proven | Actual hardware decoder/surface/output acceptance and correct failure handling; host 4K UI calculations do not qualify. |
| Live TV/DVR | Blocked until separately authorized real Plex consumer acquisition/heartbeat/switch/release acceptance | Plex remains the backend; no direct ErsatzTV connection, tuner or XMLTV/DVR implementation. No production tuning override; recording mutations remain disabled. |

Stop on a reboot, lost presentation ownership, unresolved callback/job, retained
port/decoder/GPU allocation, unverified release, privacy failure or mismatched
symbols. Preserve evidence; do not escalate to more formats, 4K or Live TV. Never
free uncertain native resources merely to make shutdown appear successful.

## Current acceptance classifications

- IMPLEMENTED / HOST-TESTED: protocol/session/UI and mixed-format synthetic decoding
  with explicit source/PCM verification and bounded resource observations.
- NATIVE-COMPILED: only after exact-source CI compile/link/sign/package validation.
- REAL-PMS-TESTED: unrun; real permissions, stream leases and DVR acceptance remain
  blocked pending separately authorized service verification.
- PS5-TESTED: unrun; physical startup/audio/video/controller/decoder/presentation and
  firmware acceptance remain blocked. Hardware hold stays active.

See [release readiness](RELEASE-READINESS.md) and the existing installation documents
for preserved scope. This plan provides preparation, not a physical GO recommendation.
