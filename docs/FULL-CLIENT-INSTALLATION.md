# Held full-client build — do not install

This package compiles the actual Plexium application (`PLEX_STARTUP_ONLY=0`),
including authentication, libraries, artwork, playback services and native guide.
It is distinct from the isolated Stage 1 build. Compilation does not establish
feature completeness, native runtime compatibility or safe console startup.

Physical hardware is unavailable. `ci/HARDWARE-HOLD` remains in force after the
reported reboot. Do not connect, transfer, install or launch this candidate.
No automatic deployment, merge or stable release is authorized.

When hardware becomes available, first resolve the original startup fault and
complete the controlled Stage 1 acceptance procedure with matching symbols.
Only then plan full-client authentication, profile isolation, 1080p playback,
stop/release and failure-recovery acceptance. Keep 4K and DVR tuning gated until
their respective resource and session contracts are verified.

Use the exact-commit build manifest, native-validation report, SHA256SUMS and
matching debug ELF. Preserve the existing `/data/Plexium/` state and historical
artifacts. Never publish account tokens, PINs or playback URLs in diagnostics.
