# Plexium

A cinematic Plex experience for PlayStation 5.

**Native development checkpoint: host tested, cross-compiled, linked, signed and package-verified. Physical PS5 and live Plex acceptance remain pending.** This is an independent project, not an in-place PlexStation rebrand. PlexStation remains the installed fallback.

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

The first end-to-end **fixture** slice reaches playback dispatch. A real native PS5 title has now compiled and passed ELF/SELF/package checks; real console playback remains unverified. The install archive is `artifacts/Plexium-PPSA85421.zip`, with matching debug ELF and a native source kit. See [delivery](docs/DELIVERY.md) and follow the coexistence checks before hardware testing. Host binaries and loose object files are not console applications.

Many requested phases remain incomplete: full metadata/related/cast pages, sort/filter/pagination, Watchlist, dedicated music/audiobooks, DVR, profile avatars in the new view, expanded playback negotiation, robust progress reconciliation, screensaver and hardware performance/acceptance. Free cloud Live TV is deliberately omitted. See [validation](docs/VALIDATION.md), [reference audit](docs/REFERENCE-AUDIT.md), [architecture](docs/ARCHITECTURE.md), [installation/rollback](docs/INSTALLATION.md), [Free Live TV decision](docs/FREE-LIVE-TV.md) and [licensing](THIRD_PARTY.md).

Source is published on the [feature branch](https://github.com/tattedbeardo89/Plexium/tree/feature/plexium-native-foundation) and [PR #1](https://github.com/tattedbeardo89/Plexium/pull/1). [Native GitHub CI passed](https://github.com/tattedbeardo89/Plexium/actions/runs/40347283259); [independent download verification passed](https://github.com/tattedbeardo89/Plexium/actions/runs/40351502414). Installation ZIP, matching symbols and sources are available in the [draft prerelease](https://github.com/tattedbeardo89/Plexium/releases/tag/untagged-4ce07098e7c8119bd150) (sign in as an authorized repository user). See [GitHub delivery](docs/GITHUB-DELIVERY.md). The PR remains unmerged; physical PS5 installation and public release remain pending. GPL-3.0-or-later; imported components retain their own notices. Original branding artwork is CC0-1.0.
