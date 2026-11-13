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
