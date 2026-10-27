# Native startup review after 0.2 launch failure

The user reports that source `75d7ecf672ed79ab165a879a9a059b899801ae56` failed to launch even when only its executable/imports replaced those of working 0.1, and the last attempt led to reboot, safe mode and system-software recovery. This is a serious user-reported outcome. Available logs contain no Plexium fault address or launch failure, so root cause is unresolved. No further console test is requested.

## Review against working 0.1

Baseline source is `2fbce98d89f830bd0df67f321fa7262296b6f643`. The published ZIPs have identical metadata/artwork/modules and modes; only executable/import-address reports differ. Both SELF containers pass structural/digest checks. The imported symbol sets are identical, but addresses/order differ. ELF entry point/type and program-header count match; sizes/addresses naturally differ. No existing check proves a console launch.

The 0.2 source did not change the renderer, platform adapter, native CRT, runtime, signing recipe or package metadata. The changed entry points add artwork/avatar wiring and playback display state; the application layer adds browsing/caches/details and cloud TV. This review also covers inherited native hazards because unchanged code can become reachable under a new workload.

## Concrete corrections

- Native startup no longer reads memory at `assumed_base + offset` from mutable `imports.txt`. Optional API/decoder availability checks read the linked image's own RIP-relative GOT. The textual report remains a build/debug artifact. The previous pointer reads could fault if a report was stale or the load base differed; neither condition has been established on this console.
- Removed automatic helper-daemon requests, global allowlist writes, shared jailbreak request-file deletion, broad media-directory discovery and shared `/data/plexium` creation. Plexium observes launcher-provided access and uses `/app0` without requesting a sandbox transition. These inherited paths existed in 0.1 too; removal is a containment correction, not a proved reboot fix. Private storage must remain accessible through the launcher's supported sandbox; unavailable access is reported instead of requesting expanded privileges.
- Startup opens its log before redirecting descriptors. Failed log creation leaves the existing stderr stream valid, addressing undefined use after a failed `freopen`. Tests exercise failed and successful opens.
- Fatal reporting uses prepared descriptors and bounded numeric formatting. It no longer opens files, calls stdio/thread helpers, dereferences guessed native context offsets or scans a damaged stack inside a signal handler.
- The frame loop checks wait/acquire/reset/begin/end/submit/present results. Wait/acquire calls have a five-second bound. Fences reset only immediately before submission, and processing stops after a rejected submit or device loss. The entry point returns on graphics failure instead of retrying indefinitely. Fault-injected tests execute the production frame code. This does not make all driver/video waits bounded or establish kernel recovery behavior.
- Startup phase messages identify platform/graphics/UI/player/session initialization without logging credentials. Matching symbols and source identity remain required.
- `ci/HARDWARE-HOLD` prevents native delivery/failure-report scripts from replacing the quarantined candidate draft. CI investigation artifacts can be retained, but no new build is recommended for installation. Original 0.1 remains untouched.

## 0.2 feature preservation

| Requested change | Source retained / verification scope |
| --- | --- |
| Home avatars | Primary-account-scoped artwork lane, cache and UI callbacks; fixture checks only |
| Video overlay | Four-second hide/wake; paused/buffering controls stay visible; shared-view regressions |
| Remote Home speed | Progressive rows, request suppression, scoped warm caches and remembered authorized HTTPS endpoint; fixture checks |
| Movie/episode details | Poster/backdrop, cast pages, related shelves, parent-show inheritance and full description; fixtures/shared view |
| Movies/TV navigation | Recommended, Library, Recent, Unwatched, Genres, Collections; bounded pages and correct collection/playlist routes |
| Music/audiobooks | Artwork fallback, richer metadata, artist/album/track views and individual-track playback; full queues/cross-chapter progress remain unfinished |
| Free Live TV | Selected-profile direct provider adapter retained as experimental; signed-in access, native video/audio, ads/session handling unverified |
| Performance | Bounded library pages and caches retained; no measured PS5 speed claim |

All application-layer changes from 0.2 remain in the source. This does not turn unfinished features or fixtures into completed device acceptance.

## Validation and remaining blocker

Host suites, ASan/UBSan, native-import resolution tests, log-open failure tests, injected Vulkan failure tests, production transport loopback scenarios and a fresh native compile/link/sign/package validation are required for this review. The final validation results are recorded after completion. No hardware execution, firmware repair, registration database change, public release or PR merge is performed.

A kernel/launcher crash record or an existing Plexium log from the failed attempt may help if already available after recovery. No reproduction is needed to collect this evidence. Without fault/module evidence or independent native compatibility verification, the reviewed build cannot be asserted to launch like 0.1, and is held from replacement delivery.
