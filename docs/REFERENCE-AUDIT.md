# Reference audit — 2026-10-25

| Reference | Revision inspected | Evidence and limits |
|---|---|---|
| PlexStation default `feature/plexstation-initial-implementation` | `e79d5bd3a882cb6c841de4d0a8cd084bd1eb840a` | Read README, protocol interfaces, native main and build/dependency recipes. Not the newest relevant source. |
| PlexStation `feature/ps5-ui-artwork-stabilization`, PR #1 | `a0a56086004858107f33dcb81d1e890b215fd382` | Source exported read-only for selective reuse. Last application commit `53a766675c42d5b4f6b1496f94c3b56ad32a1653`. Newer initial-layout, authorized routing and Home ordering fixes included. |
| Jelly5 `main` / v0.7.0 | `b9d45a5b2c1571ee7c220fb541f3a15a0fb8e1f4` | Inspected Makefile, native Home, navigation, details, login and artwork interfaces; uses its EVO/AGC engine, custom drawing and FFmpeg/native decoder. GPL-3.0. No copied code/assets. |

`reference-build-evidence.json` preserves upstream's reported successful native run 40211142558 and exact built revision. This is **reference documentation**, not an independently downloaded/verified artifact or a Plexium build. No GitHub Actions API result was obtained in this task. Physical claims remain the upstream owner's reports, not our hardware validation.

| Capability | Finding | Evidence classification |
|---|---|---|
| Device link, Home/PIN, account persistence | Selectively reused client/auth/profile helpers; original orchestration | Source verified, new session fixture tested; live account untested |
| Authorized server selection | Identity check, section request, profile-scoped access tokens | Source verified and fixture tested |
| Home feeds and episode identity | Stabilization branch has a closed six-slot Home model and parent artwork fields | Source verified; retained Home regressions and new policy tests executed |
| Native video/audio | Existing VLC video-memory output, Vulkan textures, PS5 decoder and timestamped audio | Source inspected; upstream build/owner evidence only; Plexium native execution untested |
| Artwork | Source includes bounded CPU/GPU/disk caches and profile scoping | Source inspected, host syntax only for native integration |
| Broader music, audiobooks, Watchlist, DVR | Reference has substantial helper/UI implementations | Source paths exist; not carried forward as functional Plexium screens |
| Jelly5 visual ideas | Featured hero, scroll-away Home, large focus targets, incremental art and details | Source inspected; independently implemented patterns, not visual/code transplantation |
| Firmware 13.40 | Requested target | Untested for Plexium |

Plexium retains the tested Plex protocol boundaries and native decoder substrate to avoid replacing proven playback with an unrelated engine. Jelly5's AGC/FFmpeg runtime is a different stack and was not mixed into VLC. Original branding and the new UI were implemented separately. The complete per-file upstream SHA-256 inventory is `reuse-manifest.json`; selected tests preserve regression assertions with identity strings adapted for Plexium.

Title candidate `PPSA85421` is formatted like the references, differs from PlexStation `PPSA85301`, upstream VLC `PPSA85300`, and Jelly5 `PPSA99505`. Package content ID, concept ID and runtime paths were changed consistently. This is a local reference comparison, **not a global allocation or console collision check**. Check installed titles and both data paths before installation. No registration database was accessed.

The requested Plexium name was retained. No authoritative trademark database search or global repository-name clearance was completed; existing uses and trademark availability remain unverified. The app identifies itself as unofficial and uses original artwork, rather than Plex's marks as its icon. A repository search would not establish legal clearance.

## Naming search

A public GitHub repository search for `Plexium` was retrieved on 2026-10-25. It returned existing names including [Clarit-AI/Plexium](https://github.com/Clarit-AI/Plexium), [Batu1-1an/Plexium](https://github.com/Batu1-1an/Plexium), [plodroid/Plexium](https://github.com/plodroid/Plexium), and the requested owner repository. The selected name is therefore not globally unique among repositories. This search does not establish trademark availability or rights; no authoritative trademark-registry clearance was obtained. The user's product name is retained with an explicit unofficial-client statement.
