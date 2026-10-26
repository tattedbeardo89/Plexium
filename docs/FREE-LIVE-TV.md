# Plex Free Live TV: OMIT from this checkpoint

Decision date: 2026-10-25. This optional service does not block the core personal-server client. No Free Live TV destination, hidden L1/R1 route, fake playback or official-app placeholder is exposed. `src/app/capabilities.h` records the disabled boundary.

| Evidence | Classification | Consequence |
|---|---|---|
| User's official web player can play free channels | User-reported service availability | Does not establish an authorized independent API or PS5 decoder path |
| PlexStation inspected reference docs/source at a0a5608 keep free cloud TV unavailable | Source/documentation verified | Reuse neither browser secrets nor undocumented endpoints |
| No current official permission/session/ad/DRM contract was obtained | Unverified | No third-party integration entitlement claim |
| Plexium native decoder not cross-built or run on PS5 | Untested | No HLS, ad-transition, real audio or channel-switch success claim |

No Plex cloud channel endpoint was probed. No cookies, official-app identities, geographic/device restrictions, DRM, ads or access controls were bypassed. No stream was recorded, cached or redistributed. The official overview and terms were subsequently retrieved as recorded below. An authorized third-party session interface and maintained-community implementation audit remain unverified; missing evidence is not interpreted as a blanket service denial.

Reevaluate only with an authorized maintainable interface and permitted real tests. Required acceptance remains listing/EPG, two allowed channels with real video/audio, correct ad/session behavior, expiry/renewal/reconnect, region/account rejection and cleanup. Synthetic lifecycle tests cannot establish those facts. Personal DVR is separate and not wired into this checkpoint either.

## Official public sources retrieved

On 2026-10-25 the following official pages were successfully retrieved over verified HTTPS:

- [Free Live TV Streaming on Plex Overview](https://support.plex.tv/articles/free-live-tv-streaming-overview/) describes ad-supported channels, supported Plex players, and the distinction between free channels and a user's DVR source. It documents official-player use, not a demonstrated third-party PS5 playback/session/ad contract.
- [Plex Terms of Service](https://www.plex.tv/about/privacy-legal/plex-terms-of-service/) describes service/content licensing and restrictions. This task does not treat accessible content URLs or ordinary account access as an integration grant.

These sources support retaining the permission/technical gate. They do not establish an independent client entitlement or a working native cloud-TV path. No legal clearance, DRM status or stream compatibility is inferred. A maintained-community implementation audit and explicit authorized real-session testing remain outstanding. Decision stays **OMIT**; core personal-server development continues independently.
