# Artwork workers and guide parsing — 2026-11-13

The rebuild retains PR #1's authenticated Plex, Home/PIN, library, renderer and
VLC sources, superseding default-stack artwork jobs with checked pthread lanes.
No additional reference code or third-party assets are imported.

## Implemented

- Three artwork lanes use 1 MiB stacks, with at most one outstanding result each.
  Cache maintenance shares a media lane. Failed worker creation produces a
  bounded failed-image result and retry delay; failed maintenance remains queued
  for a later retry. No credentials, metadata paths or image URLs are logged.
- Job teardown joins before callback/result destruction. Profile generation
  checks discard obsolete image results before GPU upload.
- Guide parsing rejects malformed successful HTTP responses rather than
  marking them as successfully refreshed empty pages. Explicit empty containers
  remain valid. Guide time filtering precedes the per-response item cap.
- Production artwork tests exercise three concurrent fetch/decode jobs, refused
  fourth work, profile changes during fetch, native upload boundary thread
  affinity and texture retirement. Worker tests cover invalid stack requests,
  busy lanes, reuse and joining during destruction. These are Linux tests.

## Existing native evidence

Code commits: `2c5b56b` (artwork) and `c8d597e` (guide).
Local validation passed: `bash host/check.sh` and
`SANITIZE=1 bash host/check.sh` (11 C++ suites, curl HTTP/TLS, seven rendered
frames, branding, diagnostics/finalizer, imports and private startup logs);
both startup modes under host syntax with exceptions/RTTI disabled;
actual Stage 1 entry injection; Vulkan frame/shutdown injection; and ten
production GnuTLS/socket HTTP/TLS cases. An initial auxiliary transport command
lacked the Vulkan header path; rerunning with the pinned headers passed.

Local `ci/build-native.sh` stops before compilation: `PS5_VULKAN` and the pinned
media dependency installation are absent. New push-triggered native CI is
required for this checkpoint's exact source identity. No new native executable,
installation ZIP, symbols or checksum is claimed by this local validation.

The older `REBUILD-DELIVERY.md` pending-build entry is historical. On this audit,
GitHub reports successful native run
[43369637474](https://github.com/tattedbeardo89/Plexium/actions/runs/43369637474)
for baseline `2dbe1200c46b29bc42e078a662cabc24c7a727ea`, including native
compile/sign/validation and artifact uploads. This does not validate the artwork
changes in this checkpoint. PR #2 remains draft; PR #1 is open and unmerged.

The baseline Stage 1 installation artifact is available from that run as
`Plexium-0.5.0-stage1.2dbe1200c46b.zip` (inner ZIP SHA-256
`8180d896fe3f05c203fd8721d3a0f26b6c95e38becf1c08f3abe3fb54cb44221`,
as recorded in PR #2). It is startup-only, initializes no artwork or playback,
and is not a complete Plex client. No baseline artifact was downloaded again
or rebuilt locally during this audit.

## Limits

No physical PS5 or authorized real Plex/DVR instance was used. Stage 1 excludes
the full application's artwork workers; a successful Stage 1 build cannot
establish their native runtime behavior. Full-client native compilation and
staged hardware acceptance remain necessary. Existing console fault attribution,
decoder resource reclamation, verified Live TV consumer/session lifecycle,
transcoding, recording UI and complete feature parity remain open.

`ci/HARDWARE-HOLD` remains. Follow `STAGE1-INSTALLATION.md` and
`NATIVE-REBUILD.md`; no merge, stable release or hardware deployment is part of
this checkpoint.
