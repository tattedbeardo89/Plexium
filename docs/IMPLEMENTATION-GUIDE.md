# Project Plexium — New Independent Native PS5 Plex Client

## IMPORTANT: NEW REPOSITORY, NOT AN IN-PLACE REBRAND

This is a **new Codex chat connected to the same development environment**. Build **Plexium** as a separate, independently maintainable native PlayStation 5 Plex client, using two existing projects as references. This request **supersedes previous instructions** to rebrand PlexStation in place. Do **not** modify, rename, migrate, delete, reset, or repurpose the original PlexStation repository.

- **New project/repository:** `https://github.com/tattedbeardo89/Plexium` (create if absent and authorized; if remote creation is unavailable, initialize a new local `Plexium` repository and report exactly how to publish it).
- **Functional reference, read-only:** `https://github.com/tattedbeardo89/PlexStation`
- **Design/PS5 technical reference, read-only:** `https://github.com/02dnot/Jelly5`
- **Product name:** Plexium
- **Target platform:** jailbroken PlayStation 5, firmware 13.40
- **Existing PlexStation title ID:** `PPSA85301` — **reserved for PlexStation; never reuse it for Plexium**.
- **New title ID:** Select a properly formatted, conflict-checked candidate distinct from `PPSA85301`; document its selection, treat availability as unverified until validated, and apply it consistently to package/install metadata. Do not overwrite the original installation.
- **Positioning:** “A cinematic Plex experience for PlayStation 5.”

## New Codex chat and existing workspace handoff

The prior Codex conversation developed a UI error after pictures were pasted into the chat. The erroneous last message cannot be cleared. That is a **chat-interface issue**, not evidence of a source-code, image-asset, or console defect. Do not diagnose or repair the previous chat and do not require access to its context.

Start with the actual workspace and Git state:

1. Inspect the current working directory, all relevant Git remotes, branches, HEAD commits, working trees, uncommitted changes, existing documentation, build scripts, tests, and CI workflows. Do not assume the default branch contains the latest PlexStation features.
2. Locate the latest functional PlexStation implementation including relevant feature branches/PRs and validated build reports. Study the **actual code**, not only README or prior summaries. Preserve every original working tree and unmerged change.
3. Inspect Jelly5 source and documentation for reference. Use read-only copies outside the new Plexium tree as needed. Do not commit the reference repositories as nested Git histories or submodules unless separately justified.
4. Prepare a **capability inventory**: proven PlexStation features, stable reusable interfaces, known bugs, platform constraints, licensing, missing features; and a Jelly5 architecture/design comparison. Label each finding verified, inferred, or untested.
5. Create a **new independent Plexium project** with its own Git history, directory, CI, build/package output, README, and branding. Do not make it a branch or rename of PlexStation. Preserve provenance and all required attribution for any reused code.
6. Use **selective reuse**, not blind copying: favor proven PlexStation Plex API/auth, profile selection, server discovery, storage and playback behavior when licenses/technical compatibility permit, while independently designing a coherent new native rendering/UI architecture influenced by Jelly5. Do not recreate known-fixed bugs by starting from outdated code.
7. Do not invoke destructive repository commands against either reference. Do not force-push or rewrite published history. Do not automatically merge, deploy to hardware, uninstall PlexStation, or publish publicly. If repository creation/push or PR opening needs credentials/approval, do the available local work and state the blocker rather than guessing.
8. Implement, test, package, and report results. **Do not stop at a proposal or audit.** Use incremental checkpoints with visible tests and usable progress.

### Scope and architectural principles

- Plexium is a **Plex-only** client; Jelly5 is not its backend or protocol.
- First establish a minimal end-to-end native PS5 slice (boot → Plex device sign-in → Plex Home/server selection → authorized library → media details → playback). Then expand the UX in independently testable increments.
- Preserve PlexStation as an installed fallback; Plexium must coexist without replacing or corrupting its title, data, package registration, or caches.
- Make Plexium user-facing branding visually original. Because “Plex” is a third-party mark, include an accurate *unofficial/independent client* statement and do not imply endorsement or use proprietary Plex brand artwork as Plexium's app identity.
- Respect GPL-3.0 and other applicable license obligations if copying/adapting code from Jelly5; audit PlexStation and all linked dependencies too. Prefer independently implemented design ideas where license compatibility is uncertain. Document provenance and license notices. Do not claim a GitHub naming search grants trademark rights.
- Optimize for smooth 1080p/4K TV navigation, DualSense input, responsive artwork, secure auth, and stable playback on the actual target firmware. Distinguish host/CI success from console verification.

## Primary objective

Build and ship a **new, installable Plexium PS5 application**, with a separate repository, new package identity, premium cinematic design inspired by Jelly5, and the reliable Plex capabilities learned from PlexStation. The result must be a native console experience, not a web-only mockup, a simple fork/rebrand of PlexStation, or a Jellyfin client.

---

# Phase 1 — Study Jelly5's implementation

Clone and inspect:

https://github.com/02dnot/Jelly5

Study the actual implementation rather than relying only on the README or screenshots.

Inspect:

- Home screen
- Featured-content hero
- Movie library
- TV library
- Search
- Media details
- Season and episode browsing
- Native rendering implementation
- Input handling
- Focus navigation
- Artwork caching
- Transitions and scrolling
- Loading states
- Settings
- Authentication screens
- PS5 metadata and package construction
- Native playback architecture

Review Jelly5's screenshots, documentation, reusable design patterns and relevant source files.

Identify design and technical ideas suitable for Plexium’s new architecture, and compare them against the proven PlexStation implementation.

Compare both repositories before selecting an implementation strategy.

Produce a short design comparison documenting what will be adapted, what will be redesigned independently, and what cannot reasonably be reused.

### Licensing and attribution

Jelly5 is GPL-3.0 licensed.

Preserve required licensing and attribution for any code actually copied or adapted.

Do not copy Jelly5 branding, logos, images or screenshots into the final application.

Do not assume a design concept requires copying its underlying code.

Prefer original implementations of shared UI concepts.

Preserve all applicable licenses for existing PlexStation dependencies.

---

# Phase 2 — Establish Plexium's independent identity and project skeleton

Build a **separate** `Plexium` repository and original app/package identity. The product name is **Plexium**. Use the tagline “A cinematic Plex experience for PlayStation 5.” Include clear “Unofficial Plex client; not affiliated with or endorsed by Plex” attribution where appropriate.

- New Git history, project README, dependency/license documentation, developer setup, reproducible builds, and CI.
- Clean modular boundaries for Plex networking/auth, session state, library metadata, media playback, UI/rendering, controller focus/navigation, texture cache, persistence, and native PS5 platform integration.
- Original title icon, splash, loading assets, home-screen artwork, settings/About identity, documentation, binary and release artifact names.
- A new, consistently applied PS5 title ID **distinct from `PPSA85301`**, with a collision-check report and separate data paths; never install over or modify PlexStation.
- **No PlexStation user-data migration is required for initial Plexium installation**; users sign in independently. Optional future import must be opt-in, read-only from old data, and explicitly validated.
- Check existing software/repositories and relevant trademarks for **Plexium**, explain naming/brand risks without silently renaming the selected product, and report results. Never claim legal clearance from superficial searches.
- Never hardcode credentials, account tokens, server URLs, or secrets in repository or CI artifacts.

### Acceptance checks

Verify a native skeleton builds and installs alongside PlexStation without title-ID or save-data collisions; the shell icon and launch metadata are correctly packaged; and the app displays an interactive Plexium screen with functional controller input.

---

# Phase 3 — Create an original Plexium visual identity

Develop original artwork specifically for Plexium.

Design direction:

- Premium cinematic streaming interface
- Deep charcoal and near-black backgrounds
- Restrained violet, indigo or electric-blue accents
- Soft translucent/glass-inspired surfaces
- Subtle highlights and borders
- Clean typography
- Strong artwork presentation
- Readable text at television viewing distance
- Smooth focus transitions

The UI should resemble the visual polish of Jelly5 without being a direct visual clone.

### Branding assets

Create:

- Original Plexium wordmark
- Original application logo
- PS5 icon0.png
- Appropriate background DDS artwork
- Startup splash
- Sign-in artwork
- Loading indicator
- Favicon or related assets if applicable

Use a deterministic SVG master and reproducible raster/texture generation pipeline.

Create a compact logo that is recognizable at PS5 home-screen icon size.

Avoid Plex's official logo and Jelly5's identity.

### Home-screen artwork

PlexStation previously experienced shell artwork not updating on the physical PS5. Use that lesson to build a reliable Plexium artwork registration and verification workflow.

Investigate the complete title-registration pathway before claiming the rebrand is installed successfully.

Check:

- Packaged icon
- Mounted icon
- Staged icon
- ShadowMount+ title discovery
- Registered artwork
- PS5 shell cache
- Duplicate title registrations
- Launcher artwork references

Adapt or independently implement a read-only Windows artwork diagnostic script for Plexium, preserving provenance and license requirements.

Provide a safe, separately initiated, title-specific artwork-refresh procedure if needed.

Do not automatically uninstall either application or manipulate the PS5 registration database.

Keep backups and rollback instructions.

A green packaging test does not establish that the new icon appears on the physical PS5.

---

# Phase 4 — Build a Jelly5-inspired Home screen

Create a cinematic Home experience.

## Featured hero

At the top, display one featured item from the signed-in user's authorized libraries.

Use:

- Large backdrop artwork
- Title or logo artwork where available
- Short synopsis
- Year and content metadata
- Play or Resume
- More Info
- Subtle overlay gradients for readability

Use a stable, configurable featured-content selection policy.

Do not display inaccessible content.

Do not autoplay video previews by default.

Avoid unnecessary image downloads during startup.

## Home content order

Below the featured hero, preserve the previously requested Home section ordering:

1. Continue Watching
2. Recently Released Movies
3. Recently Released Episodes
4. Recently Added Movies
5. Recently Added TV Shows
6. Seasonal Recommendations

No duplicate rails.

Do not add an On Deck rail.

Do not add a Home Movies rail to Home.

Omit empty rails while preserving the order of remaining sections.

Allow the hero to scroll away naturally when navigating down.

### Artwork policies

Use portrait movie posters for movies.

For episode entries in Continue Watching and Recently Released Episodes, use their parent show's portrait poster, not episode video screenshots.

For Recently Added TV Shows, use the series poster.

Maintain consistent card proportions.

Preserve episode titles, season numbers, episode numbers and playback progress.

Selecting an episode must act on the correct episode, not accidentally on the series.

---

# Phase 5 — Modernize sidebar and navigation

Take inspiration from Jelly5's navigation simplicity.

Use Plexium's own coherent console navigation.

### Sidebar behavior

- Home appears first.
- Search remains easily accessible.
- Only libraries authorized for the selected Plex Home user are shown.
- Movie, TV, Music and Audiobook libraries have correct classification.
- Settings and profile controls remain accessible.
- Continue Watching is not a standalone sidebar destination.
- Rentals is absent.
- Plex Free Live TV appears only after the optional Phase 8A confirms a legitimate, maintainable integration with real authorized playback; otherwise it is absent.
- Personal DVR appears only when the current user has relevant authorized functionality.

Rebuild sidebar contents when switching Plex Home users or servers.

Preserve stable IDs and focus restoration.

### Controller behavior

- Cross selects.
- Circle returns.
- D-pad navigates.
- L1/R1 switches top-level sections where appropriate.
- Left/right navigates horizontal media rails.
- Focus always remains visible.
- Scrolling follows focus.
- Settings never traps focus offscreen.

Use a single navigation registry.

Do not allow hidden legacy destinations to appear through L1/R1.

Maintain safe margins at 1080p and 4K.

---

# Phase 6 — Redesign media libraries

Build modern Jelly5-inspired library pages for:

- Movies
- TV Shows
- Individual authorized libraries
- Music
- Audiobooks
- Collections

### Features

- Cinematic library headers
- Consistent poster grids
- Sorting
- Filtering
- Genre/category navigation
- Search within library where appropriate
- A–Z navigation when useful
- Clear empty and loading states
- Smooth scrolling and controller focus

Do not display pages that remain blank until the user presses D-pad Down.

Trigger proper native layout invalidation after state or metadata updates.

### Categories

Use the previously requested text-only category artwork.

No category pictograms.

Use attractive original typography-based SVG cards.

Use consistent sizes, legible text and subtle background treatments.

---

# Phase 7 — Redesign media details

Use Jelly5's media-detail experience as a reference for visual polish.

Create original Plexium details screens.

## Movies

Provide:

- Cinematic backdrop
- Poster/title artwork
- Synopsis
- Year, runtime, rating and genres
- Play/Resume
- Watchlist where supported
- Cast and crew
- Related Movies
- Collections or extras where available

## TV series

Provide:

- Show backdrop
- Series poster/logo
- Synopsis
- Series metadata
- Continue Watching / Play Next
- View Seasons
- Cast
- Related TV Shows

## Seasons

Provide a dedicated season page with:

- Season artwork
- Episode count
- Season description
- Episode list
- Watch progress
- Previous/next season navigation

## Episodes

Provide:

- Episode title
- Series and season context
- Episode synopsis
- Episode still as backdrop
- Play/Resume
- View Season
- View Series
- Guest cast where available
- Related TV Shows

### Cast

Use the most specific Plex-provided credits.

Do not mislabel series-level cast as episode-specific guest stars.

Show actor portraits and characters when available.

Use original fallback portraits where required.

### Related media

Use authorized Plex metadata.

Do not fabricate recommendations or require restricted cloud endpoints.

### Horizontal details rails

Implement controller-friendly scrolling rows for seasons, related titles, cast and other relevant material.

Preserve focused-card restoration when returning to the previous page.

Load secondary content incrementally.

---

# Phase 8 — Keep Plex-specific capabilities

The redesign must remain explicitly designed around Plex.

Preserve:

- plex.tv/link device authentication
- Plex Home profiles and PINs
- Plex Home avatars
- Server discovery and selection
- Single-server auto-selection
- Remembered server identity
- Authorized library access
- Plex Watchlist
- Continue Watching
- Played/unplayed state
- Resume positions
- Plex metadata and image APIs
- Audio and subtitle selection
- Supported native playback
- Personal Live TV/DVR where authorized
- Music
- Audiobooks classification
- Existing settings and persistence

Do not introduce Jellyfin or Emby login flows.

Do not substitute Jellyfin API calls for Plex endpoints.

Do not assume Jelly5 playback features automatically exist in PlexStation.

If Jelly5 provides a feature the current PlexStation stack lacks, inspect feasibility and implement it only when supported and verifiable.

---

# Phase 8A — Plex Free Live TV feasibility and optional implementation

**This phase is an investigation with an optional implementation gate, not a blocker for Plexium's core Plex Media Server client.** The user can play Plex's free ad-supported channels in Plex's official web player. That establishes availability in that player, **not** a verified third-party API entitlement or native PS5 playback path. Treat Plex Free Live TV and personal Live TV/DVR as separate services.

## 8A.1 Research and permission checks

Investigate current Plex documentation, developer terms and guidance, service-specific requirements, and maintained community projects. Verify as far as reliable evidence allows:

- Whether an independent native client is authorized to list and play free cloud Live TV channels, and whether there is an officially documented or otherwise authorized integration interface.
- The existence and stability of channel-directory, channel-logo, category, EPG/program-guide, playback-session, and stream-manifest mechanisms.
- Auth/account and managed-user requirements; country, region, device-class, and entitlement restrictions.
- Stream types (including HLS if applicable), codecs, encryption/DRM status, redirect/session expiration, and whether channel playback requires server-side ad insertion, session reporting or other service obligations.
- The difference between public evidence, officially supported behavior, implementation-dependent observations, and unverified claims.

**Do not** bypass DRM, ad delivery, device or geographic restrictions, authentication, or application access controls. Do not impersonate Plex's official application or reuse browser cookies, internal tokens, or protected session secrets. Do not claim that an accessible stream URL implies permission to redistribute or integrate it. Do not record, restream, timeshift, cache entire programs, or redistribute free channels.

## 8A.2 Community and PS5 technical feasibility

Study lawful open-source examples that explore Plex's cloud channels, including their maintenance status, protocols, dependencies, permissions, and fragility. Document provenance/licensing for any actual reuse. Do not blindly transplant undocumented URLs.

Evaluate Plexium's native player for authorized streams: HLS/adaptive playback, media codecs, A/V sync, ad transitions, buffer management, bounded reconnect, session refresh, channel-switch cleanup, artwork, guide updates, and controller input. Separate **stream format compatibility** from **permission to access the service**. Prefer isolated mock tests before live testing.

## 8A.3 Gated proof of concept

Only if a legitimate access path is established, implement an isolated, optional module/proof of concept that:

1. Retrieves an authorized channel listing with labels/categories/logos.
2. Retrieves authorized now/next program and guide information where available.
3. Initiates an authorized playback session for one allowed channel.
4. Demonstrates actual video **and audio** with correct ad/session behavior.
5. Switches between at least two permitted channels and releases previous resources.
6. Handles expiration, unavailability, session renewal, reconnect, and geo/permission rejection gracefully.
7. Uses bounded requests, no secrets in CI/logs and no stream redistribution.

Use synthetic fixtures for CI when real service access is unavailable. Never report mocked playback as successful real playback. Ask for no passwords or account tokens in tickets, logs, or chat.

## 8A.4 Conditional Plexium interface

If, and only if, authorized real playback works and the interface is maintainable, enable an independent **Free Live TV** destination with a cinematic Plexium guide: channel logos, categories, guide timeline, currently airing and next programs, favorites, buffering indicators, full-screen player, and controller channel-switching. Respect region/account access and service terms. Do not require a personal server, DVR/tuner, or Plex Pass merely to open free channels.

Keep **Personal Live TV/DVR** separately permission-gated for the user's configured Plex Media Server tuner/recordings. Never conflate the two. Do not offer recording/timeshifting for the cloud channels when unsupported or restricted.

If the integration is not both authorized and technically demonstrable, **omit Free Live TV entirely** from sidebar, L1/R1 registry, and release UI. Do not replace it with a misleading button or a placeholder instructing users to use Plex's official app. Preserve an isolated, disabled integration boundary and a concise limitations report for future reevaluation.

## 8A.5 Tests and report

Test channel metadata/guide parsing, artwork, account and region gating, absent DVR, availability failures, HLS/mock player lifecycle, cleanup, controller focus, sidebar gating, startup performance, and secrets redaction. Where legitimately possible, document real stream testing separately from mocked and host-only tests. Physical PS5 acceptance remains pending until user verification.

Deliver a clearly labeled **implement / defer / omit** decision, an evidence table citing verified sources and dates, supported or unsupported interface findings, licensing/policy and native decoder constraints, prototype code if warranted, pass/fail test logs, and exact remaining blockers. **Never delay or weaken the core Plexium release to wait for this optional service.**

---
# Phase 9 — Improve startup performance and loading

Take inspiration from Jelly5's fast native presentation.

Profile Plexium startup and use available PlexStation metrics as comparison where measured on comparable hardware.

Optimize:

- Initial native UI readiness
- Restoring saved authentication
- Remembered server selection
- Plex Home user selection
- Metadata loading
- Artwork fetching
- Texture decoding
- UI frame scheduling
- Cache reuse

Display Plexium's initial screen as early as practical.

Never block the entire UI waiting for secondary artwork or related titles.

Load the Home hero and essential navigation first, then other rails incrementally.

Use placeholders instead of blank screens.

Do not weaken authentication or TLS verification for speed.

Provide benchmark results comparing before and after.

---

# Phase 10 — Redesign sign-in and profile selection

Create a welcoming Plexium authentication experience.

### Before device linking

Display:

- Plexium branding
- Attractive cinematic background
- Prominent Sign in to Plex button
- Concise explanation of device linking
- Visible controller focus

### Linking screen

Display:

- Pairing/linking code
- plex.tv/link instructions
- QR code if supported
- Loading/authorization status
- Retry and cancellation

Do not expose secret tokens.

### Plex Home

Display user avatars, names and clear focus highlights.

Handle managed users and PIN protection.

Use appropriate initials placeholders when avatars are unavailable.

Preserve remembered server and profile behavior.

---

# Phase 11 — Playback experience

Retain the existing reliable playback engine unless profiling or verified defects require changes.

Improve the playback overlay visually to match Plexium.

Include where supported:

- Title and episode details
- Seek bar
- Resume progress
- Audio track selection
- Subtitle selection
- Chapters
- Playback information
- Next episode control
- Appropriate controller hints

Do not invent features or represent unsupported behavior as functional.

Do not copy Jelly5's media-decoding code without checking technical compatibility, dependencies, performance and licensing.

Keep playback regressions separate from purely visual redesign work where practical.

---

# Phase 12 — Screensaver and polish

Evaluate whether Jelly5-inspired screensaver behavior can be implemented safely.

If feasible, use authorized library backdrop artwork for an optional inactivity screensaver.

Requirements:

- Configurable idle timeout
- No activation during video playback
- No interruption of PIN or authentication screens
- No token exposure
- Predictable dismissal through the controller
- Low memory and GPU overhead

Keep the feature optional.

Add subtle animations and transitions where practical.

Prioritize navigation responsiveness over decorative effects.

---

# Phase 13 — Respect the existing native PS5 architecture

PlexStation already has a functional native PS5 application that serves as a **reference**, not the codebase to modify. Choose Plexium’s independent architecture after examining both references.

Compare:

- Rendering pipelines
- Native UI components
- Texture handling
- Media decoding
- Controller input
- Memory management
- Build systems
- SDK dependencies

Choose a maintainable new implementation that selectively reuses verified behavior or legally compatible code rather than transplanting entire projects.

For major architectural choices, document benefits, risks, dependency changes, validation and maintainability.

Retain the ability to debug native crashes and collect matching symbols.

Preserve firmware 13.40 compatibility and package/data isolation from PlexStation.

---

# Phase 14 — Testing

Add or update tests covering:

### Branding

- New Plexium name and original artwork
- New package identity and independent data paths
- No collisions with existing PlexStation title or saved data
- Native package assets and reproducible icon generation
- No unintended PlexStation branding on Plexium startup assets

### Navigation

- Sidebar order
- Permission-aware libraries
- Controller focus
- L1/R1 behavior
- Horizontal scrolling
- Focus restoration

### Home

- Featured hero
- Exact Home rail order
- No duplicates
- Correct poster artwork
- Loading without controller input

### Details

- Movie details
- Show → Season → Episode
- Related titles
- Cast and guest credits
- Progress indicators
- Missing metadata fallbacks

### Authentication

- Plex device linking
- Plex Home profiles
- PIN handling
- Single-server restoration
- Avatar rendering

### Playback

- Play and Resume
- Movie playback
- Episode playback
- Audio tracks
- Subtitles
- Playback progress synchronization

### Performance

- Startup responsiveness
- Bounded memory and texture use
- Image caching
- Navigation during background loading
- Stability under slow-network conditions

Run the new Plexium hosted tests, sanitizers, native PS5 compilation and package verification; port relevant regression scenarios from PlexStation.

Add actual native rendered-frame checks where feasible.

Do not weaken or remove regression tests.

Clearly distinguish host simulation from physical PS5 hardware testing.

---

# Phase 15 — New-project workflow, delivery and acceptance

Use **Plexium** as the only writable product repository. Treat PlexStation and Jelly5 as **read-only source/design references**. Protect unrelated working trees, feature branches, published histories, installed titles and app data.

Build in reviewable, testable checkpoints:

1. **Reference audits, license/provenance matrix, new repository, CI/build skeleton and distinct PS5 title identity.**
2. **Native rendering/design system, original Plexium branding, sign-in and Plex Home/server discovery.**
3. **Authorized library navigation, hero, Home rails, posters, categories and search.**
4. **Movie/show/season/episode detail pages, cast, collections and related items.**
5. **Reliable native playback, audio/subtitle controls, focus and controller handling, optional screensaver; independently investigate the gated Plex Free Live TV feature without blocking core milestones.**
6. **Performance optimization, regression suite, native packaging, artwork registration diagnostics and final documentation.**

Do not weaken tests, fabricate results, merge automatically, rewrite history, install to the user's PS5 without authorization, or publish a repository publicly without authorization. If GitHub access permits creating a **private** Plexium repo, do so only when explicitly authorized to create it; otherwise produce the local project and provide the commands/actions needed to connect it. Use a feature branch and reviewable PR when supported.

### Definition of done

- Standalone, reproducibly buildable Plexium native PS5 project with its own repository and branding.
- Consistent unique package ID, separate save/cache locations, and demonstrable noninterference with existing PlexStation.
- Secure Plex device linking, Plex Home/PIN, server selection/persistence, authorized media browsing, correct posters/metadata, functional playback and controller navigation.
- Original cinematic UI following the precise Home rail order and media-details behaviors specified above.
- Host regression tests, sanitizers where available, PS5 cross-compilation, package inspection, checksums and matching debug symbols.
- Honest list of functions verified on real PS5 hardware versus simulated/CI tested and any tests still needing hardware.
- License and attribution review for any reused material and no misleading official Plex affiliation claims.

### Final report format

Provide:

1. New Plexium repository URL or exact local path and reason remote publication is blocked, if applicable.
2. Original PlexStation reference commit/branch and Jelly5 reference commit inspected.
3. Reuse-versus-reimplementation decisions and dependency/license attribution matrix.
4. Selected **new** title ID and evidence it does not conflict with the existing PlexStation title ID `PPSA85301` (state unverified risks).
5. Architecture diagram and component boundaries.
6. Original branding previews, package icon/artwork, and screenshots from actual implementation where feasible.
7. Authentication, profile/PIN, server, library, search, metadata, cast and related-content outcomes.
8. Home rail ordering, poster/artwork, media details, controller and focus test results.
9. Playback compatibility and audio/subtitle control results.
10. Startup/performance measurements and comparisons where baselines exist.
11. All test commands, pass/fail results, native build SHA, artifact checksum, and matching debug symbols.
12. CI workflow/PR links if actually created; do not invent any link.
13. Installation procedure for **coexisting** with PlexStation, including backups and rollback.
14. Remaining known defects, blocked features (including Phase 8A Plex Free Live TV decision) and hardware acceptance tests.

## Definition of success

**Plexium is an independent, native PlayStation 5 Plex client**, inspired by Jelly5's polished console experience and informed by PlexStation's working Plex implementation. It has its own maintainable architecture and install identity, doesn't compromise PlexStation, and delivers a genuinely usable testable native build—not just visual mockups or a renamed binary.

Implement the project, validate what is possible, and produce the build artifacts. If some goals remain incomplete, make the completed subset usable and document exact next steps.
