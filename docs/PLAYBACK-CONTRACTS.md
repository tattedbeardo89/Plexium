# Playback and platform contract evidence

## Plex protocol sources

No private server or credentials were available. New production integration uses
existing Plex universal decision/start conventions, not invented DVR endpoints.

- [PMS OpenAPI decision](https://github.com/LukeHagar/plex-api-spec/blob/f39d0e39909b72cef5bc1f61e4c6195b5ae815bf/plex-api-spec.yaml#L19915)
  and start at line 50613 document client identity, capabilities, media/part index,
  HLS, bitrate, resolution and decision schemas. This community-maintained spec
  is evidence of a contract, not all-version server acceptance.
- [Plex MPV Shim media adapter](https://github.com/iwalton3/plex-mpv-shim/blob/22696d876a43ab410ff56ebfb96851dcfc1cbf37/plex_mpv_shim/media.py)
  demonstrates matching session identifiers, decision/start and universal stop,
  plus client capabilities syntax. No reference source is copied into Plexium.
- [OpenPHT keepalive](https://github.com/RasPlex/OpenPHT/blob/cf23f65fd009a163c417239a26be3cc6060c9882/plex/Client/PlexMediaServerClient.cpp)
  uses `/video/:/transcode/universal/ping` with the current session. Keepalive
  failures are visible but not classified as decoder errors.

The old Chrome profile assumption is removed. Missing/unsupported server decisions
fail closed rather than assuming HTTP 200 means playable media. Subtitle-burning
and advanced named-track renegotiation remain unsupported in this milestone.

## Native boundaries reviewed without hardware

The pinned SDK `b83202be73e930050e00de0f1b4d9ec46c0391bf` exports
`sceAudioOutClose` in `sce_stubs/libSceAudioOut.c` lines 302–304. An assembly stub
establishes symbol presence only; it does not establish parameter/return ABI,
firmware behavior or blocking bounds. The existing weak boundary and retained
handle on failure remain. Do not infer accepted audio teardown from host mocks.

Emby5 reference `53f7a0da055b1b92fa9665f7741c5c74df1ac69f`,
`app/engine/media/src/evo_vdec_native.c`, has cached decoder slots, direct-memory
compute queues and reinitialization logic. Its ownership model differs from
Plexium's VLC module. `evo_audio_out.c` supplies no verified audio-close contract.
These observations do not authorize copying cache eviction into `ps5vdec.c`.
The existing process-lifetime queue/arena and cleanup quarantine are retained.
No guessed driver/kernel calls or unverified 4K reclamation were added.

Vulkan graphics fence completion still does not establish display-engine
presentation completion. WSI/device retention remains the conservative boundary.
No new presentation-completion declaration or native VLC stop/release timing
guarantee was established. Native driver/library calls remain potentially
blocking even though owner waits are bounded. Host simulations do not resolve
these contracts; independent Plex feature development continues regardless.

## Next acceptance layers

1. Expand live-server-independent fixtures for named audio/subtitle selection,
   durable scoped progress reconciliation and next-episode playback.
2. When an authorized PMS is available, test real HLS Direct Stream/transcode,
   session expiration/release and server watch-state behavior; record version.
3. When a real personal DVR is available, verify consumer acquisition/release
   before enabling production tuning. Keep Plex as the only DVR backend.
4. When PS5 hardware returns, resolve the original startup fault, presentation,
   audio and decoder release contracts, then verify repeated 1080p playback.
   4K and Live TV runtime acceptance follow their own gates.

## Confirmed host plugin lifetime boundary

Ubuntu 24.04 CI reported 12,384 bytes in six allocations after all three real
playback sessions passed. Loader address evidence identified Pixman 0.42.2,
loaded through Cairo by VLC. Reproducing that exact Pixman package locally
reproduced the same six allocations. Its published source (`pixman/pixman.c`,
`pixman-implementation.c`) constructs `global_implementation` and a fallback
chain at library load and exposes no public destructor for that global chain.
Unloading the plugin dependency loses the cache's roots.

The host executable now owns Pixman as a direct dependency and verifies its
version before VLC initialization, preserving the documented process lifetime
of its implementation cache. It also finalizes its owned Fontconfig context
and pauses OpenMP resources after VLC releases. This is a host platform boundary,
not a native PS5 change, manual destruction of private library objects or an
LSan suppression. The exact failing Ubuntu runtime passes the final normal and
ASan/UBSan runs with leak detection enabled. Loader traces remain available in
CI diagnostics so future runtime-specific failures can be attributed.

## Pinned native WSI source finding

The actual RADV-linked backend is
[PS5 Mesa `wsi_common_videoout.c`](https://github.com/mihawk-99/PS5_Mesa/blob/1ba9874f478657ccf6da72ce55f0260164cc0bd0/src/vulkan/wsi/wsi_common_videoout.c),
not the separate experimental `PS5_Vulkan/driver` implementation. Its opening
contract describes process-lifetime registered VideoOut framebuffer sets.
`videoout_swapchain_destroy` waits with `cnd_wait` while `chain->in_flight` is
nonzero. The present-wait hook delegates to the generic semaphore helper; this
source review does not establish a bounded application-facing display-release
contract. Existing WSI retention is therefore preserved. SDK
`platform/include/ps5platform/videoout.h` declares the VideoOut functions and
flip-status layout based on reference console observations, not Plexium hardware
acceptance. No new direct VideoOut calls or cache reclamation are added.
