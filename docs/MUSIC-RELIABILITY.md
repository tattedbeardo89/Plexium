# Music, reconnect and memory reliability

This extends the actual Session/View on the existing rebuild branch. Exact-source
host/native results and held artifacts belong to draft PR #2. No real PMS, private
network, account credentials or console testing is performed. Hardware hold and
production DVR tuning/recording mutation gates remain unchanged.

## Queue and persistence

Queues retain at most 2000 compact tracks and 8 MiB of accounted storage. Duplicate
songs have separate monotonically assigned entry IDs; ordering is independent of
metadata identity. Move/remove/select commands use entry IDs plus a runtime scope
epoch, never an old UI row index. Editing another row preserves the active entry.
Removing the active row first stops/releases it, then plays its successor if present
(or wraps with Repeat All). An incomplete stop retains ownership and blocks edits
that would replace playback. Clear stops music before clearing. Album/playlist/artist
additions stage admission so a failed load/budget cannot partially alter the queue.
Larger catalogs remain browseable in pages; Play Artist refuses an incomplete or
more-than-2000-track queue rather than silently claiming all tracks were queued.

`music-sessions-v1.json` is private, owner-only, schema-versioned and atomic through
the existing checked fsync/rename helper. It allows 16 account/profile/server scopes
and 2 MiB total. Records contain scope, rating keys, section IDs, queue-entry IDs,
order/current entry, offset and shuffle/repeat. No tokens, URLs, artwork, descriptions
or duplicate large metadata are persisted. Playback/language/focus preferences remain
in their existing scoped private settings. Unsupported schemas/corrupt files are not
overwritten. An unconfigured/unsafe store does not prevent in-memory playback.

After account/profile and server validation, removed music sections are pruned and
current metadata is checked before Resume is offered. Other restored entries are
validated in bounded batches; absent/inaccessible entries are removed. Unsupported
bulk responses stop background checking after three failures; every selected track
still requires authorized metadata before opening, using fresh resolution or a
matching, revision-bound pre-resolution no older than thirty seconds. Profile changes discard
validation/prefetch state. Protected Home users retain the existing PIN gate.
Launch never starts audible Music automatically.

## Native controls and audio focus

Native input stages Square/Options/Triangle/L1/R1 until release or a chord. Held
modifiers are combined with the new direction/button and consumed without a solo
action; blocked/context-changed input is discarded. This avoids removing a track
before a reorder chord and preserves guide/player combinations.

The queue is available in navigation and via Options+Right from Music/background
views. Cross plays/resumes a selection, Square removes it, Square+Up/Down moves it,
Triangle moves it next and Options clears it; L1/R1 page. Artist Square plays and
Triangle shuffles; Options+Square adds later, Options+Left adds next for artists,
albums, playlists and tracks. Music tabs include recently added albums and recently
played tracks. Options+Triangle opens the named-track picker from Music playback.
Unsupported selections preserve current audio instead of opening an unverified
conversion. Albums/artists use Plex children/leaves and bounded pages.

The mini-player shows title/artist, artwork and progress while browsing Home, movie,
TV, search, Music, settings and guide. Options+Cross pauses, Options+L1/R1 changes
track, Options+Down opens now playing and Options+Right opens queue. Existing guide
recording/favorite chords remain. Video takes the single player only after Music
stops/releases; the queue and observed offset survive. Resume after natural video
end is opt-in and defaults Off. Manual video Stop does not auto-resume Music.
Paused/failed/retained sessions do not create a second output session.

Next-track resolution uses the same serial worker: one metadata item, 30-second TTL,
128 KiB media admission and 256 KiB accounted total. Queue revision, entry identity
and context invalidate it; Repeat One may reuse the same valid item within its TTL.
Failed pre-resolution backs off ten seconds. It never
opens a player, decoder or transcoder. Routine music timelines are throttled to ten
seconds; controls and Stop remain immediate. Queue offsets save at most every fifteen
seconds during normal observation, plus edits/Stop/shutdown. This improves transitions
but does not prove sample-continuous gapless audio; no speculative PS5 APIs/reuse exist.

## Reconnection and authorization

A periodic HTTPS identity/library check detects unavailable/revoked access. Failures
schedule 2/4/8/16/32/60-second backoff, with six attempts then a visible paused circuit.
Settings can explicitly retry. Each attempt considers at most three authorized
routes, with the previous route first. Requests have short per-call bounds; no render-thread
network work is introduced. Reconnect validates primary identity, the current Home
user, fresh authorized resources and the SAME server machineIdentifier before choosing
an HTTPS route. It never invokes automatic selection of an unrelated server.

Authorization denial clears private UI/history/queue views, stops playback, closes
the artwork credential context and requires profile authentication again. Pending
unproven server release is retained. Temporary network failure preserves selection,
queue and offset. Failed interrupted Music can reopen after successful validation if
the continuation preference is enabled; paused Music stays paused. Restored app-launch
sessions still require explicit Resume. Pending progress goes through fresh-server
reconciliation before retry, not blind replay over newer server progress.

## Audio fallback boundary

Primary references: [Plex PMS API](https://developer.plex.tv/pms/) documents metadata
leaves/multi-ID metadata, transcode type music and universal decision/start interfaces;
[Plex's Kodi audio player](https://github.com/plexinc/plex-for-kodi/blob/master/lib/_included_packages/plexnet/plexplayer.py)
shows the music HLS start request. These establish negotiation/start shapes, not this
client's complete music keepalive/stop/release contract.

Unsupported audio now receives a read-only music decision check with strict item,
version, part and AAC output validation. Network/server/identity/incompatible output
and an accepted-but-held conversion receive distinct errors. The decision is not a
consumer lease. No music transcoder start, guessed stop endpoint, duplicate transcode
session or override is enabled. Actual conversion and native codec acceptance remain
pending; existing verified video negotiation is preserved.

## Accounted memory budgets

| Retained state | Budget / policy |
| --- | --- |
| Page snapshots | 8 MiB / 12 entries, oldest loaded evicted; single admission <=4 MiB |
| Metadata items | 4 MiB / 32 entries; existing <=512 KiB media admission retained |
| Navigation history | 8 MiB / 16 snapshots; oldest evicted |
| Home warm cache | 2 MiB; oversized warm copy discarded, current view preserved |
| Personal lineup / optional cloud metadata | 8 MiB / 4 MiB; refreshable cache copies only |
| Music queue | 2000 entries / 8 MiB; compact display/identity state |
| Next-track metadata | One item / 256 KiB accounted; no audio buffer or player |
| Artwork decode | 2 million pixels / 8 MiB RGBA per lane; three existing lanes |
| Artwork output/GPU/disk | Existing resized output, 64 MiB GPU, scoped bounded disk limits preserved |
| Command requests | Existing 16 queued commands; Tick coalesced, one Session worker |

Accounting includes retained item/container capacities, string capacities and JSON
node/payload costs; it is not a measured total process/RSS, allocator or native driver
memory bound. Admission staging, parse temporaries, two model snapshots and artwork
codec scratch also consume bounded independent storage. History/page snapshots omit
transient queue/now-playing/notices copies. Disk-hit artwork reuses its validation
decode instead of decoding twice. GPU/presentation/decoder safe retention is unchanged;
active native resources are never freed to satisfy a metadata budget.

## Acceptance

`music-reliability-tests` exercises 10000-track browsing, 1200 albums, 2000 entries,
10000 identity-preserving edits, duplicate songs, persistence/schema/profile isolation,
restore without autoplay, next metadata reuse, background navigation, single-player
focus, retained-stop rejection, reconnect backoff/revocation and byte eviction before
entry-count limits. Existing suites remain. Actual host VLC additionally covers queue
edits and persisted resume/reopen using local AAC fixtures. Native View raster coverage
expands from 26 to 35 distinct screens. ASan/UBSan, NDEBUG, TLS, GPU/entry/player/audio/
decoder checks and both native modes remain required. None establish REAL-PMS-TESTED
or PS5-TESTED acceptance, true gapless continuity or the original reboot cause.
