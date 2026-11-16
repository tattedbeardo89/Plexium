# Rebuild reference audit — 2026-11-12

PR #1 was read through GitHub metadata and its fetched source. It remains open,
non-draft and unmerged at `d2b5e7cc01aa4162375e938e6ec1e0e9beeb96fe`. Its
hardware hold supersedes historical package success. Preserve the Plex protocol,
authentication, Home/PIN, discovery, UI/artwork and pinned native build work;
supersede its always-full bootstrap and unverified personal-Live-TV URL shortcut.
The new branch descends from that head, preserving all prior commits.

Read-only reference checkouts inspected:

| Project | Exact revision | Findings |
|---|---|---|
| [Emby5](https://github.com/bornaradusin/Emby5) | `53f7a0da055b1b92fa9665f7741c5c74df1ac69f` | README, GPLv3 license/notices, 1.3.1 release notes and `app/engine/media/src/evo_vdec_native.c`: small boot slots, on-demand 4K growth, idle-slot eviction, restoration owned by the active slot. Different decoder ownership from Plexium's VLC module. |
| [Jelly5](https://github.com/02dnot/Jelly5) | `333e393b7f1a5b98d88099b4782dab97bd10fa5d` | App Makefile, engine licensing, playback/audio clock and native decoder source layout. EVO/AGC architecture; no Jellyfin implementation imported. |
| [EVO Player](https://github.com/sainsaji/EVO-PLAYER-PS5) | `08bcab5de76cb7507ecc0bc3e1d4bca9bea13788` | README, GPLv3 license, projects and media engine layout. Native decoder and rendering approach is a reference, not verified firmware compatibility for Plexium. |

No source or assets from these three checkouts were copied. Existing imported
VLC/Vulkan components retain their existing provenance and notices in
`THIRD_PARTY.md` and `docs/reuse-manifest.json`. No additional license clearance
or hardware capability claim follows from source inspection.

[Plex PMS OpenAPI 1.2.3](https://developer.plex.tv/pms/) was checked for DVR
listing, tune, provider grid and session playlist operations. Responses require
shape checks. A successful tune alone does not prove a consumer playlist can be
played or released correctly. The implementation therefore leaves tuning off
until the full authorized lifecycle can be tested. Discovery/grid code is
isolated in `src/plex/live_tv.h`; typed bounded presentation lives in `guide.h`.
[docs.plex.tv](https://docs.plex.tv/) has a public index and private documentation
that was not accessed.

[ErsatzTV's Plex setup instructions](https://ersatztv.org/docs/clients/plex/)
confirm server-side tuner/guide setup in Plex. Plexium does not connect to
ErsatzTV or handle its credentials, schedules, XMLTV or recording files.

Source review does not establish physical 4K/HDR, AV1, HDMI bitstream, DRM,
firmware 13.40, Relapse, etaHEN/kstuff or ShadowMountPlus compatibility. Emby5's
4K resource manager has not been ported into the retained VLC architecture.
