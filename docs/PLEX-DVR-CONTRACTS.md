# Plex DVR integration evidence and gates

Starting source: `33245f765e8f4058384d2f1243d911a294d530cb`.
No authorized PMS/DVR credentials or captures were supplied. No console is available.
The implementation remains a Plex frontend: libraries → ErsatzTV → Plex DVR → Plexium.
It never contacts a tuner or ErsatzTV directly and creates no DVR infrastructure.

## Confirmed documentation and source contracts

Primary reference: [Plex PMS OpenAPI 1.2.3](https://developer.plex.tv/pms/), reviewed for this milestone.
Requests use the currently authorized profile's `X-Plex-Token`, stable client identifier,
and explicit JSON Accept header over verified HTTPS. Authorization/transport errors
remain sanitized. The real-server runner uses the production Client/cURL adapter.

| Operation | Contract | Evidence/limits |
| --- | --- | --- |
| DVR discovery | GET `/livetv/dvrs` | Official; Dvr entries carry a key. No device configuration is changed. |
| Provider discovery | GET `/media/providers` | Official; authorize provider DVR IDs against discovery before following provider keys. |
| Lineup | GET `/{epg-provider}/lineups/dvr/channels` | Existing provider adapter; bounded pagination now uses Container-Start/Size. Channel IDs, grid keys, names, numbers and logos remain server supplied. |
| EPG | GET `/{epg-provider}/grid?channelGridKey=...&date=...` | Existing provider contract; viewport filtering, midnight dates, bounded pages and cached offline state retained. |
| Channel tune | POST `/livetv/dvrs/{dvrId}/channels/{channel}/tune` | Official 200 metadata / 500 tuning-failed. The example contains `Media.uuid`, stream descriptions and `live=true`; it does **not** establish an owned consumer grant. No production call is enabled. |
| Session read | GET `/livetv/sessions` and `/livetv/sessions/{sessionId}` | Official metadata reads. Reading metadata is a health probe, **not proof of consumer keepalive**. |
| Delivery | GET `/livetv/sessions/{sessionId}/{consumerId}/index.m3u8` | Official HLS, 404 when session/consumer is absent. The controller accepts a compatible, same-origin, identity-bound URL from a validated protocol adapter; it never manufactures one from a tune UUID. |
| Session deletion | DELETE `/livetv/sessions/{sessionId}` | Confirmed in the [plexpy SDK source](https://github.com/LukasParke/plexpy/blob/main/src/plex_api_client/live_tv.py), not present in the reviewed official Live TV section. Whole-session versus consumer-only cleanup and effects on shared viewers require real-server evidence. |
| Recording rules/schedule | GET `/media/subscriptions?includeGrabs=1&includeStorage=1`, GET `/media/subscriptions/scheduled` | Official subscription APIs; permissions/Plex Pass are enforced by PMS, with 403 possible. |
| Completed recordings | GET `/livetv/recordings` | SDK source contract; older servers can reject it. The schedule remains visible with an explanation if unsupported. |

The SDK is supporting implementation evidence, not an authorized real-server capture.
No code was copied from it. Tests use original synthetic metadata.

## Implemented controller and explicit missing contracts

`LiveTvSession` owns acquisition, buffering, playing, stopping, released, error and
recovery states. It requires a validated protocol adapter, compatible stream grant,
session/consumer identity and same-PMS playlist before opening a player. It tracks
previous channel, bounded heartbeat retries, explicit stop, shutdown and idempotence.
A failed player stop or release retains ownership and blocks retuning. Release-pending
recovery cannot falsely resume Playing. An unknown acquisition outcome blocks another
allocation until a future verified adapter reconciles ownership.

`DvrRequests` implements the documented tune/read shapes and SDK deletion shape with
bounded production Client calls. `PmsLiveProtocol` deliberately has no validated
consumer acquisition/keepalive/release implementation and cannot be enabled by an
environment variable or UI setting. The production Session wires this gate into
Tune, channel switching, Last Channel, Stop, context changes and shutdown.

Still BLOCKED: how this client's consumer is allocated, the exact keepalive/timeline
parameters and expiry behavior, compatible live transcode negotiation and ownership
of that additional lease, consumer-only versus shared-session release, ambiguous
POST outcome reconciliation, and proof that tuner allocations return to baseline.
Do not treat a universal VOD transcode stop, a metadata GET or an SDK whole-session
DELETE as proof of these contracts. Tuning, recording creation and cancellation stay
disabled until authorized evidence verifies the required semantics.

The deterministic harness uses production DvrRequests and a **fixture-only**
LiveProtocol that supplies synthetic consumer grants. Its health pulse uses the
session metadata GET solely to exercise recovery; it is not a verified PMS heartbeat.
Successful fixture acquisition is not REAL-PMS-TESTED or PS5-TESTED behavior.

## Native guide and recording UI

The guide supports up to 1600 deduplicated channels across eight bounded lineup pages,
six visible channels, current/upcoming airings, logos, descriptions, time navigation,
Jump to Now and program progress. It rejects repeated pagination and indicates partial
results. Favorites and their filter are private atomic schema-1 local preferences
scoped by account/profile/server (64 scopes, 512 favorites per scope, 256 KiB file).
They do not modify Plex's channel configuration. Malformed or unknown preference files
are preserved. Mini Guide rendering is independent from tuning availability.

Controls: Square toggles a favorite; L1+R1 toggles favorites filtering;
Square+Triangle toggles Mini Guide; Square+Options opens recording rules, scheduled
and completed recordings. Recording requests/cancellation remain disabled. During a
future validated live session, Up/Down select channels, Square selects Last Channel,
and Triangle opens the guide while the worker continues servicing the live controller.

## Evidence labels

IMPLEMENTED/HOST-TESTED: controller ownership/error paths, synthetic protocol adapters,
640-channel production Session flow, scoped favorites, Mini Guide, read-only recordings,
EPG errors/offline/expired-auth paths and production tuning zero-allocation gate.
Real VLC tests additionally exercise local HLS for fixture Direct Stream/transcode
fallback decisions and actual ended/next-episode reopen. These are not server-transcoder tests.

REAL-PMS-TESTED: skipped without explicit secure configuration; see REAL-PMS-TESTING.md.
NATIVE-COMPILED: only the successful matching native CI manifests establish this for a commit.
PS5-TESTED: none. Hardware hold stays. Original reboot cause, display-engine presentation
completion, native audio-close ABI, decoder cache/arena release contracts, hardware 4K
and native playback acceptance remain unresolved.
