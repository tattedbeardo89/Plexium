# Plexium 0.2 browsing update delivery

**Hardware hold:** the latest 0.2 launch test was followed by reboot/safe-mode recovery. Do not install or retest 0.2. No replacement is hardware accepted. See [startup review](STARTUP-AUDIT.md).

**Known 0.2 installation regression:** the user reports that 0.2 does not appear in ShadowMountPlus, while restoring 0.1 works at the same path. Keep 0.1 installed. See [diagnosis](SHADOWMOUNT-REGRESSION.md); 0.2 is not a confirmed hardware-compatible update.

The [open, unmerged PR #1](https://github.com/tattedbeardo89/Plexium/pull/1) contains fixes for avatars, automatically hiding video controls, progressive Home loading and warm caches, paginated Recommended/Library browsing, richer details/cast/related shelves and audio artwork/metadata. See [browsing update](BROWSING-UPDATE.md).

Compiled source: `75d7ecf672ed79ab165a879a9a059b899801ae56`. [GitHub native build](https://github.com/tattedbeardo89/Plexium/actions/runs/40399618166) passed host tests, ASan/UBSan, PS5 compilation/link/signing and package validation. [Independent download verification](https://github.com/tattedbeardo89/Plexium/actions/runs/40402550151) passed checksums, ZIP contents/modes, matching debug identity, source-kit manifests and repository bundle validation.

Download the [PS5 installation ZIP](https://github.com/tattedbeardo89/Plexium/actions/runs/40399618166/artifacts/12688861934), [matching debug ELF](https://github.com/tattedbeardo89/Plexium/actions/runs/40399618166/artifacts/12689292012), [source kit and repository bundle](https://github.com/tattedbeardo89/Plexium/actions/runs/40399618166/artifacts/12689536586), and [checksums/build record](https://github.com/tattedbeardo89/Plexium/actions/runs/40399618166/artifacts/12688482687). Actions artifacts expire after 90 days. The same verified files are attached to the separate **0.2 draft prerelease** on the [releases page](https://github.com/tattedbeardo89/Plexium/releases); sign in with repository access.

Follow [installation and rollback](INSTALLATION.md): back up the existing app and private data, replace the complete PPSA85421 app folder, retain `/data/plexium-client`. Original 0.1 release and matching symbols are preserved. PlexStation and reference checkouts were untouched; no console deployment, PR merge or public release occurred.

The user reports the original 0.1 icon, UI and video work. This new update still needs PS5 acceptance, particularly remote-server loading and artwork. Free Live TV is experimental: anonymous provider probing returned HTTP 401; authenticated profile access, native picture/audio, ads and session expiry are unverified. Full album queues and cross-chapter audiobook progress remain broader work. See [Free TV limits](FREE-LIVE-TV.md) and [validation](VALIDATION.md).

The environment configuration draft saves reusable setup/start instructions; saving does not apply or publish it. Fresh-task restoration has not been independently verified.
