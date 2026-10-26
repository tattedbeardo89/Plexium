# Build and development

Use `/workspace/Plexium`, the existing isolated checkout. Do not create worktrees unless requested.

## Verified host tools

The current machine provides GCC/G++ 14.2, make, pkg-config, libcurl 8.14.1, GnuTLS 3.8.9, Python 3, OpenSSL and Pillow 12.3.0. Host tests use C++17. Artwork authoring is pinned in `tools/requirements.txt`; other machines should create a venv and `pip install -r tools/requirements.txt` with TLS enabled. No account, server or credentials are required for fixtures.

```sh
cd /workspace/Plexium
bash host/check.sh
SANITIZE=1 bash host/check.sh
VULKAN_HEADERS=/workspace/deps/Vulkan-Headers/include bash host/check-native-source.sh
```

Vulkan-Headers is a dependency checkout, not another product repository. Inspected pin `409c16be502e39fe70dd6fe2d9ad4842ef2c9a53` (v1.4.313). Host syntax checks require those headers plus GnuTLS headers. Make limits parallelism to two jobs by default; normal and sanitizer objects/binaries are separate. No local web preview/service is used.

The actual shared native view renders to `build/{welcome,linking,home,library,details,profiles}.png`. These frames use synthetic metadata. `build/plexium-cli PRIVATE_DIRECTORY` runs the real host Plex transport and session orchestration; `link` shows a device code, `poll` checks authorization, then `profile ID`, `server ID`, `library ID`, `details ID` and `children ID` drive the same protocol session. Protected PIN entry requires an interactive terminal and disables echo. The tool refuses to simulate native playback. Do not give tokens through command arguments or chat. Live operation needs the configured domains and the user's own account authorization; it was not performed during this task.

## Verified native build

Read [native preparation evidence](NATIVE-PREPARATION.md). All 22 dependency archives passed published checksum checks; all 23 prerequisite recipes, the open SDK, byte-identical runtime, patched release RADV and static VLC built. Plexium's new application compiled and linked, its homebrew SELF passed integrity inspection, and the complete package passed identity, artwork, plugin, CRC and mode checks. This does not establish firmware compatibility or real playback.

Outputs:

- `artifacts/Plexium-PPSA85421.zip`: complete `PPSA85421` homebrew folder, **not a retail PKG**.
- `artifacts/Plexium-native-debug.elf`, symbol/map reports: matching unstripped native ELF and diagnostics.
- `artifacts/native-validation.json`, `artifacts/SHA256SUMS`: exact source stamp, ELF build ID and hashes.
- `artifacts/Plexium-native-source.tar.gz` and `Plexium-source.bundle`: application history, pinned toolchain source and verified dependency inputs.

The native application was built from `36abbd8b5a9bbf9c0afda47af1142ec702669ab2`. Subsequent documentation/source-kit changes do not change its C++ sources. The first link exposed one missing retained crash-checkpoint bridge; it was supplied by the new entry point. Container Git reads now trust only the exact mounted checkout, so the built source stamp is verified in the debug ELF. No placeholder executable or host binary is substituted.

## Start native development

The pinned Arch toolchain image is retained as `/workspace/deps/plexium-ps5-prepared.tar` with a checksum. It has signed packages and no baked session proxy values or session CA material. The Dockerfile package step succeeded but BuildKit failed preparing a later filesystem layer; the tested direct-container fallback is retained. Source/build caches are under `/workspace/deps` and `/workspace/plexium-work`.

```sh
cd /workspace/Plexium
native_session=$(bash ci/start-native-container.sh)
docker exec -e PS5_VULKAN=/workspace/deps/PS5_Vulkan -e VLC_PS5_WORK=/workspace/plexium-work "$native_session" bash ci/build-native.sh
# Stop only the process you created when finished:
docker stop "$native_session"
```

The helper creates a fresh process with current proxy settings and a read-only CA mount and checks the prepared image ID. Fresh-container compiler/HTTPS checks, image-archive integrity, cached prerequisite bootstrap and the actual native title build were exercised. Retained dependencies do not require rebuilding for ordinary application changes. Live processes must not be assumed to survive publication. Restoration in a new cloud task has not been independently verified; preserve the source bundle because the local branch has not been pushed.

For a full dependency refresh use `ci/bootstrap-sdk.sh`, `ci/bootstrap-native-runtime.sh` and the dependency recipes, respecting the pinned inputs. `ci/finish-native-build.sh` performs the font-dependent prerequisites, static VLC and full title packaging; it was executed through the real build. Do not run it merely to rebuild UI code, since those dependencies already exist. The source fetcher validates both cached and new archives. Never disable TLS/signatures or change expected hashes to get past a failure.

Package ZIP timestamps derive from SOURCE_DATE_EPOCH or the source commit. Repackaging the same payload/source produced an identical ZIP checksum. This verifies archive repeatability, not a second cold rebuild of every native dependency.

Host and manual self-hosted native workflow files are committed locally. Neither workflow was pushed or executed remotely. A future native runner needs the prepared dependencies; no remote CI/PR result is claimed.

## Local publication

The product checkout is `/workspace/Plexium`; its origin already points at `https://github.com/tattedbeardo89/Plexium.git`. No push, PR, merge or public publication was performed. Once reviewed, publish the local feature branch with `git push -u origin feature/plexium-native-foundation` and open a PR if the repository's branch setup supports one. The remote was empty when inspected, so a base branch may need to be established through the owner's normal workflow.
