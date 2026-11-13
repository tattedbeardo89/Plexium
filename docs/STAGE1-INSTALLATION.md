# Plexium Stage 1 investigation candidate — HARDWARE HOLD

This archive is a native startup diagnostics candidate, not the complete Plex
client. No account, artwork, VLC player or hardware decoder is initialized.
It displays renderer/controller status and exits after holding Options + Circle
for one second. Native build success does not prove a safe PS5 launch.

**NO-GO for physical installation or launch while ci/HARDWARE-HOLD remains.**
Do not repeat the historical 0.2 launch failure. Keep existing installation and
rollback files unchanged. No automatic installation or console access is allowed.

Review `build-manifest.json`, `native-validation.json`, `SHA256SUMS`, matching
debug ELF and native stack/import evidence. Confirm the source SHA matches the
validated workflow HEAD and `startup_only` is true. The manifest outside the ZIP
records the actual build timestamp; the manifest inside it uses the source date
for reproducible package content. A source ZIP is never an installation ZIP.

Only after independent review, resolution of the existing launch-fault gate and
explicit authorization for controlled physical Stage 1 validation:

1. Close Plexium completely and preserve the working installation for rollback.
2. Verify the downloaded files with `sha256sum --check SHA256SUMS` (or an equivalent
   trusted SHA-256 tool). Inspect the ZIP: it must contain `PPSA85421/`.
3. The future installation target is `/data/homebrew/PPSA85421/`. Do not overwrite
   mounted/running application files. Preserve `/data/Plexium/` and legacy state.
4. A controlled test is limited to startup, renderer, controller and graceful
   exit. Record source identity, existing logs and the outcome. Do not enable
   full-client playback or infer 4K/firmware compatibility from the test.

No physical test has been performed or authorized by generating this candidate.
