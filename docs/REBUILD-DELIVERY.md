# Rebuild delivery checkpoint — 2026-11-12

- Branch: `feature/plexium-native-rebuild`
- Code commit: `f16b6a3b7238277921b1fabd93a4a8dd2501b972`
- Source/audit checkpoint: `f674a5b3ff1885e240103acde2795668ea6c1e72`
- [Draft PR #2](https://github.com/tattedbeardo89/Plexium/pull/2): open, unmerged.
- [Source ZIP](https://codeload.github.com/tattedbeardo89/Plexium/zip/f674a5b3ff1885e240103acde2795668ea6c1e72): **source only, not installable**.
- Downloaded source ZIP SHA-256:
  `f98ca8b915f55ee02497c684173768b0b21e5670f1bcd67a0143c05f6a53bbce`
- Download verified: ZIP CRC passed; 527 entries; source checkpoint directory
  matches the requested commit. A later documentation-only commit may appear at
  branch head; it does not change the tested code.

## Validation

Nine host C++ suites, ASan/UBSan, seven shared-native frames/controller checks,
ten curl and ten production GnuTLS HTTP/TLS cases, both native startup modes
under host syntax, and ordinary/instrumented native-entry and Vulkan failure
injection passed. See `rebuild-validation.json` for local log digests.

Local native build: blocked before compilation by absent pinned toolchain/media
dependencies. [Remote native CI](https://github.com/tattedbeardo89/Plexium/actions/runs/43358805720)
was in progress when this record was written; no completion, SELF, native stack
result, matching symbols or installation ZIP is claimed. Its artifact section
is the location for any eventual investigation ZIP/debug/source/checksum output.
It must pass all native validation gates; CI success still does not establish
console safety. `ci/HARDWARE-HOLD` remains and prevents draft-release mirroring.

Physical PS5: unrun; no console contacted. Real Plex account/DVR execution: unrun.
No stable release, deployment, merge or replacement of the old install occurred.

## Scope and next step

Implemented code includes an isolated Stage 1 entry, bounded session stack and
queues, failure-aware exit handling, private persistent state/migration, a
personal PMS DVR adapter, and native cable-style guide. Existing authentication,
Plex Home, browsing, artwork and VLC playback sources were retained from PR #1.
Their hardware acceptance is not renewed by host testing.

This is not the complete requested client. Verified Live TV consumer/session
playback and release, idle-decoder eviction/4K restoration, full transcoding
negotiation, recording UI, music queues, expanded settings, complete resource
teardown and staged physical validation remain open. Full details and hardware
stages are in `NATIVE-REBUILD.md`; licensing/research is in
`REBUILD-REFERENCE-AUDIT.md` and the existing third-party notices.

Next: inspect remote native compilation and its stack/import/package results,
then address the previous launch fault using existing evidence and independent
review before any controlled Stage 1 hardware test. Do not install this source
archive or treat CI artifacts as a normal application update.
