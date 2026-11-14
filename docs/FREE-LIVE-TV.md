# Plex Free Live TV — experimental direct-provider adapter

This update replaces the earlier OMIT decision at the user's request. The app now has a Free Live TV (Beta) destination which queries `https://epg.provider.plex.tv/lineups/plex/channels?includeMetadata=1` with the selected profile's Plex token and Plexium's own persisted client identity. It displays supported channels in bounded pages and opens only provider-returned, same-origin `/library/parts/` paths using the existing native VLC adaptive/HLS substrate. Stream paths remain private to Session rather than model snapshots. It does not send personal-server timeline events for cloud channels.

Explicit DRM/key-system/license markers are rejected, as are foreign stream origins and path traversal. No official-player identity, shared token, third-party stream proxy, region spoofing, ad removal or DRM bypass is used. Provider ad/session behavior is left intact. Account/region/client rejection is displayed as unavailable; it is not replaced with a placeholder stream.

## Evidence and remaining acceptance

- Original protocol adapter fixture tests cover channel parsing, selected-profile credentials, same-origin parts, DRM/foreign URL rejection, playback dispatch and absence of personal-server timeline traffic.
- The existing native static plugin table includes `vlc_entry__adaptive`. This establishes a compiled streaming substrate, not proof of live playback.
- The cloud worker's direct EPG request was blocked by its egress proxy (CONNECT 403); no personal account credentials were requested or inspected. Required Plex provider/image domains were added to the saved environment draft; saving does not activate network changes.
- GitHub CI records an optional compatibility report using a fresh anonymous **Plexium** identity. That report distinguishes metadata availability from signed-in profile, native video/audio, ad-transition and session-expiry acceptance. It never requests or records a stream or prints a token.
- Real signed-in channel availability, two-channel audible playback, switching, ads, expiry/reconnect and account/region rejection still require PS5 testing. A successful fixture/build or anonymous metadata response does not establish these outcomes.

## Public implementation research

Protocol shape was checked against [kptv-fast's Plex provider at 4a28a20](https://github.com/kpirnie/kptv-fast/blob/4a28a206febd17d4fce03cc29f8684bf7126012b/providers/plex_provider.py) and [SlyGuy Plex Live 0.2.4 at d11ff11](https://github.com/matthuisman/slyguy.addons/tree/d11ff115e4db10f8168873ea2c38551ac27c1f90/slyguy.plex.live). No code, artwork, credentials, official-browser headers, proxy URLs or geographic overrides were imported. Plexium uses an original adapter and its existing account/profile authorization. These are community-observed APIs, so Plex may change or reject them.

Official service overview: https://support.plex.tv/articles/free-live-tv-streaming-overview/ . Personal-server DVR remains separate from Plex's free cloud channels.
