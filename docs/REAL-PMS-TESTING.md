# Optional authorized PMS checks

Run `bash host/test-real-pms.sh`. With no configuration it reports SKIP REAL-PMS,
makes zero requests and exits 77. Fixture CI remains independent. Use a dedicated test
profile and synthetic media added to your own Plex library. No recording is created.

Set `PLEXIUM_TEST_CONFIG` to a private owner-only (0600), regular, non-symlink JSON
file, preferably under the ignored `.private/` directory. Keep its parent directory
0700. The file is bounded to 32 KiB. Never commit credentials or server metadata.
Configuration keys:

- `server_url`: your verified HTTPS PMS origin, with no URL path or query.
- `server_token`: the authorized test profile's PMS token.
- `machine_id`: expected machine identifier, checked before library access.
- `account_token`, `profile_id`, `pin`: optional primary account/dedicated profile
  authorization; protected profile PIN is supplied only through the private file.
  Fresh profile-specific discovery supplies its PMS token after switching.
- `media_id`: optional numeric rating key of dedicated non-destructive fixture media.
- `allow_watch_write`, `allow_stream_selection`: false by default. Set either to true
  only when explicitly authorizing mutations of that dedicated fixture/profile.
  Both additionally require account-token/profile authorization. The runner refuses
  ordinary watch-state or stream-selection writes without those conditions.

Alternatively set `PLEXIUM_TEST_PMS_URL`, `PLEXIUM_TEST_PMS_TOKEN`,
`PLEXIUM_TEST_MACHINE_ID`, `PLEXIUM_TEST_ACCOUNT_TOKEN`, `PLEXIUM_TEST_PROFILE_ID`
and `PLEXIUM_TEST_MEDIA_ID`. Environment values override the corresponding file keys;
there is no environment switch to enable tuning or mutations.

The read-only default checks PMS identity/library authorization, optional plex.tv
account/Home/resource discovery, fixture metadata and resume fields, audio/subtitle
enumeration, a compatible playback decision without starting the stream, DVR discovery,
lineup, first-channel guide and recording schedule. Unconfigured optional portions
report SKIP individually. An authorized watch-write test submits the same existing
resume position as a paused timeline; even this can change server activity/history,
so it requires the explicit mutation flag. The selection test reapplies only the
server's current selected IDs. Real DVR acquisition/release stays gated.

Output contains status labels rather than tokens, PINs, addresses, response bodies,
media titles or playback URLs. Strict TLS and redirect policy remain unchanged.
No personal server configuration, captures or metadata is embedded in artifacts.
