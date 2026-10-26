# Licensing and provenance

Plexium's original code is GPL-3.0-or-later. Selected GPL-compatible code was adapted from tattedbeardo89/PlexStation at a0a56086004858107f33dcb81d1e890b215fd382; that project derives its native player from ZiZc3/VLC at a91113b9731241dbfc159fd671290ff42afb2db3. Original source comments and SPDX notices are retained, including GPL-2.0-or-later notices. The per-file original hashes are in docs/reuse-manifest.json; identity strings, application integration and new interfaces were subsequently changed. Upstream credits are retained in docs/VLC-UPSTREAM.md.

| Component retained | License / location | Use |
|---|---|---|
| PlexStation Plex client, auth/Home/cache helpers, selected tests | GPL-3.0-or-later | Adapted protocol and regression components |
| VLC-PS5 runtime, shaders, platform glue | GPL-compatible upstream notices | Native rendering, decoder and timestamped audio substrate |
| Dear ImGui | MIT, third_party/imgui/LICENSE.txt | Shared native UI draw lists |
| volk | MIT, third_party/volk/LICENSE.md | Vulkan dispatch |
| stb | MIT/public-domain, source headers | Image decode |
| nlohmann/json 3.11.3, including PS5 numeric locale patch | MIT, third_party/nlohmann/LICENSE.MIT | JSON parsing |
| TinyXML-2 | zlib, third_party/tinyxml2/LICENSE.txt | XML identity/protocol parsing |
| Nayuki QR Code generator | MIT, third_party/qrcodegen/LICENSE | Retained auth regression fixture support |
| AMD FSR1; Anime4K; FSRCNNX | MIT; MIT; LGPL-3.0, adjacent notices | Retained VLC upscaling/shader substrate |
| Inter; Selawik; DejaVu and fallback fonts | Adjacent OFL/DejaVu notices in assets/fonts | UI/subtitle fonts and fallback glyphs |
| libcurl | curl license, system dependency | Host transport only |
| GnuTLS | LGPL-2.1-or-later, separate source dependency | Verified native TLS |
| VLC, FFmpeg and other static native dependencies | Each upstream license, ps5/deps/build-deps.sh | Not built/bundled as binaries in this checkpoint |
| PS5_Vulkan and pinned payload SDK/Mesa forks | See each pinned source's licenses | External open toolchain dependencies, not proprietary Sony SDK |

Jelly5 b9d45a5b2c1571ee7c220fb541f3a15a0fb8e1f4 (GPL-3.0) was inspected for design and architecture comparison. No Jelly5 source, artwork, screenshots or Jellyfin protocol implementation was imported. Original Plexium branding geometry and generated category SVGs are dedicated under CC0-1.0; bundled fonts retain their licenses.

Before distributing a statically linked native binary, retain every linked dependency's notices and provide complete corresponding source for that exact build, including patches and build recipes. The application-only archive created here is not complete corresponding source for an unbuilt native binary. No Sony, Plex or VideoLAN endorsement is claimed.
