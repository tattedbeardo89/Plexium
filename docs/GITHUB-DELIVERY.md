# GitHub delivery

[Feature branch](https://github.com/tattedbeardo89/Plexium/tree/feature/plexium-native-foundation) · [PR #1](https://github.com/tattedbeardo89/Plexium/pull/1) (open and unmerged). Main is the initially empty review baseline; existing implementation history was preserved without force pushes.

[Native build and four 90-day Actions artifacts](https://github.com/tattedbeardo89/Plexium/actions/runs/40399618166) · [Independent download verification](https://github.com/tattedbeardo89/Plexium/actions/runs/40402550151) · [Draft prerelease](https://github.com/tattedbeardo89/Plexium/releases). Sign into an authorized GitHub account for draft assets. Actions downloads normally require sign-in too.

| Download | File |
| --- | --- |
| PS5 installation | [Plexium-PPSA85421.zip](https://github.com/tattedbeardo89/Plexium/actions/runs/40399618166/artifacts/12688861934) |
| Matching debug symbols | [Plexium-native-debug.elf](https://github.com/tattedbeardo89/Plexium/actions/runs/40399618166/artifacts/12689292012) |
| Complete native source kit | [Plexium-native-source.tar.gz](https://github.com/tattedbeardo89/Plexium/actions/runs/40399618166/artifacts/12689536586) |
| Repository history | [Plexium-source.bundle](https://github.com/tattedbeardo89/Plexium/actions/runs/40399618166/artifacts/12689536586) |
| SHA-256 checksums | [SHA256SUMS](https://github.com/tattedbeardo89/Plexium/actions/runs/40399618166/artifacts/12688482687) |

Native build source: [75d7ecf672ed79ab165a879a9a059b899801ae56](https://github.com/tattedbeardo89/Plexium/commit/75d7ecf672ed79ab165a879a9a059b899801ae56). ELF build ID: `ca072e3b015b8caf5e14be67b76d67ca64149227`. native-validation.json and SHA256SUMS identify the installed bytes; current repository documentation can be newer than the compiled candidate. Source-kit documentation is retained as it existed at that build commit.

The portable native workflow uses an Ubuntu 24.04 GitHub-hosted runner and Docker. It verifies archives with the runner's TLS stack and bounded retries, prepares signed pinned Arch packages, bootstraps the open SDK/runtime and patched RADV/static VLC, compiles and validates the real native title, and uploads installation/debug/source/delivery artifacts separately. It creates its own paths and caches; no Codex filesystem or prepared image is required. Missing/uploaded artifacts fail validation. Retention is 90 days, subject to repository limits.

Relevant feature pushes trigger native builds. Manual CLI dispatch uses `gh workflow run native.yml --repo tattedbeardo89/Plexium --ref feature/plexium-native-foundation`; branch dispatch was independently exercised with the diagnostic workflow without merging implementation to main.

Direct cloud uploads returned HTTP 401, so GitHub's repository-scoped Actions token uploaded the independently downloaded/verified bytes to the existing draft. Updates explicitly preserve the candidate tag, title and source target. No public release is created. Physical PS5 firmware 13.40, live Plex playback and broader feature acceptance remain pending; follow [installation](INSTALLATION.md).
