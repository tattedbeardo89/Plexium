# Browsing and responsiveness update

The user reports that the original candidate shows the correct PS5 home icon, and that the UI and video playback generally work. The following are code/fixture improvements; this new build still needs console acceptance.

- Plex Home avatars are connected to the native artwork downloader in the chooser and active-profile header, with a primary-account-scoped private disk cache. Account headers stay on authorized Plex origins.
- Video controls hide after four seconds without input, wake on controller activity and remain visible while paused. Audio tracks retain a cover/title/artist/album screen.
- Home publishes rows as they arrive, skips redundant fallback queries for rows already supplied by server hubs, caches warm Home for 60 seconds and preserves partial results without presenting normal request-budget exhaustion as a fatal banner. A populated-hub fixture needs two requests instead of the previous section-by-section fallback sequence.
- Verified HTTPS server routes are remembered per profile and retried first only if still present in fresh resources; failed endpoint probes are bounded. TLS and identity/library verification remain mandatory.
- Movie and TV libraries default to their server's recommended hubs. Top tabs expose Library, Recent, Unwatched, Genres and Collections. Browse pages contain 60 items, with next/previous controls, rather than downloading thousands of entries. Recommendations/details cache for three minutes within the selected profile/server scope.
- Movie/episode/show details request poster/backdrop art, rating/content information, full scrollable descriptions, cast/roles/portraits and related/child rails. Episode details can inherit missing poster/cast metadata from a cached parent show. Data absent from Plex is labeled unavailable rather than invented.
- Music/audiobook libraries have recommended shelves plus Artists/Authors, Albums/Books, Tracks/Chapters and Genres. Cards use media-aware shapes and album/artist artwork fallbacks. Details include biography/summary, artist/album metadata and child tracks. Supported individual audio parts now use the native audio direct-play adapter. Complete album queues and cross-chapter listening management are not claimed.
- Free Live TV is experimental; see FREE-LIVE-TV.md for provider behavior, tests and acceptance limits.

The reported server is remote, with approximately 3,700 movies, 21,000 TV episodes, several hundred songs and 50 audiobooks. Request-count, pagination and warm-cache regressions cover the relevant scaling behavior. No numerical PS5 startup/frame-rate or remote-network speedup is claimed without measurement on that server.
