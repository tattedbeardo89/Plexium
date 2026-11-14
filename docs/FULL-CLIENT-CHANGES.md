# Playback application milestone: changed files

Production source:

- `src/app/session.h`
- `src/app/session.cc`
- `src/app/view.cc`
- `src/main.cc`
- `src/plex/streaming.h`
- `tools/host-vlc.h` (host-only software decoding adapter)

Tests, fixtures and rendering evidence:

- `tests/fixtures/plex_server.h`
- `tests/playback_integration_tests.cc`
- `tests/host_vlc_tests.cc`
- `tests/stage1_packaging_tests.py`
- `tools/render-host.cc`
- `host/test-vlc-integration.sh`
- `host/check.sh`
- `Makefile`

Native build/delivery and CI:

- `.github/workflows/host.yml`
- `.github/workflows/native.yml`
- `ci/hosted-native.sh`
- `ci/finalize-stage1.py` (now explicitly validates either requested build mode)

Documentation:

- `docs/ARCHITECTURE.md`
- `docs/FEATURE-COMPLETION.md`
- `docs/PLAYBACK-CONTRACTS.md`
- `docs/FULL-CLIENT-INSTALLATION.md`
- `docs/FULL-CLIENT-CHANGES.md`

No decoder/kernel/driver implementation, toolchain pins, state root, hardware
hold, prior artifacts or historical commits are removed by this milestone.

### Named streams, persistent progress and episode continuation

This milestone adds the shared native track picker before and during playback. Plex stream IDs remain distinct from native VLC ES IDs: the adapter resolves bounded language/name/channel descriptors and rejects ambiguous matches. Supported text captions use embedded tracks or authorized PMS sidecars; unsupported subtitle formats require a verified server burn decision. Server negotiation uses the documented `PUT /library/parts/{id}` stream selection and universal-transcode subtitle policy. Language, quality and autoplay preferences are stored per account/profile/server.

Watch updates now use a private, atomic schema-1 journal, bounded to 128 records and 256 KiB with a 24-hour expiry. Shutdown records the last observed position. Restored sessions become stopped updates; delayed retries re-read current metadata and discard conflicting/newer server progress. Authorization failures suspend retries until an authorized reconnect. The application does not overwrite unknown journal schemas. No token or playback URL is stored in the journal.

Completed episodes discover the next unwatched episode in Plex order, show a cancellable ten-second countdown, refresh its resume metadata and release the previous player before opening it. Intro/credits skip uses only Plex-provided markers. The native renderer includes track selection, preferences and continuation screenshot fixtures in addition to the existing library, artwork, profile and guide coverage.

Live TV guide browsing remains available through Plex. Tuning remains disabled: no authorized DVR server was supplied to verify acquisition, channel switching and termination. These flows must not be represented as completed fixture-based tuning. No direct tuner or IPTV fallback was added.

Host validation includes durable restart/reconciliation, scoped state, named-stream negotiation, episode transitions and twelve distinct native-view screenshots. The isolated actual-VLC job additionally generates two audio languages plus embedded/external captions and checks selections through the shared Session. All host evidence uses synthetic media and does not establish physical PS5 behavior. Both native modes must retain their separate installation/debug/manifest/checksum artifacts and HOLD status.

Protocol references: https://developer.plex.tv/pms/ (set part stream selection and universal transcode parameters); VideoLAN libVLC 3.x media/player track APIs. Physical PS5 decode, real PMS account/profile behavior, real transcoder caption output, and DVR lifecycle remain integration gates. Existing software H.264/AAC 1080p validation makes no 4K decode claim.
