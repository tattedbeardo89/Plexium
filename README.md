# Plexium

A cinematic Plex experience for PlayStation 5.

**Development checkpoint: shared native UI and Plex session foundation, host tested. A PS5 binary has not been built or verified.** This is an independent project, not an in-place PlexStation rebrand. PlexStation remains the installed fallback.

Unofficial Plex client; not affiliated with or endorsed by Plex, Sony or VideoLAN.

## What is implemented

- Original C++17 UI: welcome/device link, profiles/PIN entry, server choice, Home hero and rails, authorized library grid, search keyboard/results, basic details, settings and playback controls.
- Background session worker; real Plex device-link/profile/server/metadata calls; secure local persistence; per-profile remembered server IDs; stale result rejection and context clearing.
- Stabilization-branch Plex protocol components, closed six-slot Home order, parent-series episode poster selection while preserving episode actions.
- Selectively retained native PS5 Vulkan/VLC decoder and timestamped audio substrate, a new main loop and isolated texture integration. These runtime paths are not validated by host fixtures.
- Original SVG/PNG/DXT1 branding, provisional `PPSA85421` identity, `/data/plexium-client` state isolation, source/license inventory and manual native build skeleton.

## Run the verified host workflow

```sh
cd /workspace/Plexium
bash host/check.sh
```

This executes application/protocol/auth/Home fixtures, real local HTTP/TLS scenarios, shared-native rendered frames, controller checks and deterministic artwork tests. It writes PNGs to `build/`. `SANITIZE=1 bash host/check.sh` uses separate instrumented objects/binaries. See [build instructions](docs/BUILD.md) for native-source checks, tooling and the real host protocol CLI.

## Status and delivery

The first end-to-end **fixture** slice reaches playback dispatch; real playback remains unverified. The reference decoder does not turn a fixture into a working PS5 build. The SDK/runtime/driver and seven target object files are verified; full linking is blocked on the remaining FreeType download and dependent static VLC build. Do not install a host binary on the console.

Many requested phases remain incomplete: full metadata/related/cast pages, sort/filter/pagination, Watchlist, dedicated music/audiobooks, DVR, profile avatars in the new view, expanded playback negotiation, robust progress reconciliation, screensaver and hardware performance/acceptance. Free cloud Live TV is deliberately omitted. See [validation](docs/VALIDATION.md), [reference audit](docs/REFERENCE-AUDIT.md), [architecture](docs/ARCHITECTURE.md), [installation/rollback](docs/INSTALLATION.md), [Free Live TV decision](docs/FREE-LIVE-TV.md) and [licensing](THIRD_PARTY.md).

No repository push, public release, hardware installation or PR has been performed. GPL-3.0-or-later; imported components retain their own notices. Original branding artwork is CC0-1.0.
