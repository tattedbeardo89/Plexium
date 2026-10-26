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

## Native preparation, checks and remaining gate

Read [native preparation evidence](NATIVE-PREPARATION.md). The pinned open SDK, converter, byte-identical verified runtime, patched release RADV and nineteen target dependency recipes are prepared. Seven Plexium sources cross-compile with the real `-target x86_64-sie-ps5` wrapper. This is not a linked/signed application. The selected source dependencies are all checksum-pinned; twenty-one of twenty-two archives are verified. FreeType remains unavailable through the current runtime's permitted destinations. Its downstream HarfBuzz/libass/Blu-ray chain and static VLC build remain incomplete.

The directly launched Arch toolchain is saved as `/workspace/deps/plexium-ps5-prepared.tar`, with a checksum and expected image ID. It contains signed packages, without baked session proxy values or session CA material. The Dockerfile's signed-package step worked, but BuildKit failed on a later filesystem layer; the tested direct-container fallback is retained. Native build state is under `/workspace/deps` and `/workspace/plexium-work`, outside product sources. Do not delete those caches or repeat expensive valid builds unnecessarily.

Start a fresh process with current proxy settings and CA trust:

```sh
cd /workspace/Plexium
native_session=$(bash ci/start-native-container.sh)
docker exec "$native_session" bash ci/check-native-objects.sh
# When finished with the process you created:
docker stop "$native_session"
```

The startup helper was exercised using a fresh container, compiler discovery, current-CA HTTPS to GitHub and image-archive checksum verification. It does not rely on a process surviving publication. In a fresh task, restoration of the source checkout/snapshot must still be verified; no fresh-task restoration has been claimed.

To unblock the final native dependency chain, review/save the draft's network requirement for `ftp.cc.uoc.gr`, the mirror supplied by Savannah for FreeType 2.13.3. Automatic Savannah/SourceForge mirror redirects also hit denied hosts, so the FreeType URL is pinned to that specific upstream mirror. Existing allowlisted VideoLAN mirrors do not contain this exact version. Never change the expected checksum, disable TLS or substitute an unverified archive. If the user supplies the exact official archive securely through an authorized route, verify the already recorded digest before use.

Once actual access works, start a fresh native container and run `docker exec "$native_session" bash ci/finish-native-build.sh`. It verifies all archives, builds only the four missing dependency recipes, configures static VLC and invokes `ci/build-native.sh`. That final resume script is shell-checked but **blocked/unexecuted past the missing source**. The full title attempt currently fails on missing `build-vlc-ps5/src/.libs/libvlccore.a`.

The native recipe produces `dist/PPSA85421` with a signed homebrew SELF, not a retail PKG. On successful full build, `ci/build-native.sh` inspects it, packages `artifacts/Plexium-PPSA85421.zip`, retains the matching native debug ELF/maps/imports and writes SHA256SUMS. Those complete application artifacts do not exist yet. Do not install target object files or host binaries on the console.

`ci/build-lock.json` and `ci/source-checksum-provenance.json` record the exact toolchain/source pins. Native scripts retain checks on SDK, runtime and dependency archives. Original dependency versions are unchanged; NASM/zlib archive compression/source mirror choices were adjusted only where published checksums were available. Complete corresponding source and all dependency notices are required before distributing a final native binary.

Host CI is committed. The optional native workflow is manual and requires a user-provisioned self-hosted `plexium-ps5` runner with its prepared dependencies. Neither workflow was pushed or executed remotely; no CI links are claimed.

## Local publication

The product checkout is `/workspace/Plexium`; its origin already points at `https://github.com/tattedbeardo89/Plexium.git`. No push, PR, merge or public publication was performed. Once reviewed, publish the local feature branch with `git push -u origin feature/plexium-native-foundation` and open a PR if the repository's branch setup supports one. The remote was empty when inspected, so a base branch may need to be established through the owner's normal workflow.
