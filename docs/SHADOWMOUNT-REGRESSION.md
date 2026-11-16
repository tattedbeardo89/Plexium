# 0.2 ShadowMountPlus installation regression

The user reports the original 0.1 candidate installs and plays on their PS5. After deleting its folder and copying the complete 0.2 folder to `/mnt/ext1/homebrew/PPSA85421`, the title did not appear in ShadowMountPlus or the home screen. Discovery refresh and restart did not help. Restoring the original 0.1 contents at the same location installed successfully. **Do not treat 0.2 as hardware accepted or a confirmed replacement. Keep 0.1 as the working rollback.**

## Checks completed

The downloaded release ZIPs each contain the same 148 file paths, with identical archive file modes. Only `eboot.bin` and the informational `imports.txt` differ. Registration metadata, title/content IDs, all artwork, modules, fonts, certificates and notices are byte-identical. Both executables pass the pinned `ps5-native-tool self --inspect` integrity check: signed plaintext, twelve segments, authority `0x3100000000000002`, program type 1, app/firmware version 0. Both embedded ELF headers are x86-64 with the same entry point, executable type and program-header count. The same 481 imported symbol names occur in both reports; addresses and order differ. These checks establish format integrity, not console compatibility.

| Executable | Bytes | SHA-256 |
| --- | ---: | --- |
| Working 0.1 | 65077945 | `1bcda8075604fbe35590505f3b3b2bc146f67356bf5cd35906d1568a36438dd1` |
| Failing 0.2 installation | 65183545 | `799a6905051bd3f6f1a6d59f4a3d8824de2e664469ea42623149cf838d9531d8` |

ShadowMountPlus upstream [source inspected at c371efd](https://github.com/drakmor/ShadowMountPlus/tree/c371efd9efbb6d058cfe3600755abb37d99d4086) discovers folder candidates from readable `sce_sys/param.json` or `param.sfo`; `sm_scan.c` calls `directory_has_game_param` and `get_game_info`. Executable-presence checks in `sm_filesystem.c` use filesystem metadata. Those checks do not parse SELF signatures. This upstream revision is not yet confirmed as the user's installed version. Later PS5 registration can still fail independently.

The rollback result identifies a reproducible difference in the update procedure/package outcome. It does **not** prove the new executable is the cause: console-side file hashes, permissions, mount state and registration errors are not yet available. No signing or metadata workaround is justified by current evidence. No replacement binary is claimed to fix this issue.

## Reboot and recovery report — 2026-10-27

After replacing the executable and imports in the working 0.1 folder, the user reports launch failure, a console reboot, safe mode and a need to reinstall system software. The supplied ShadowMountPlus log ends after startup synchronization; it has no Plexium launch/crash record. It cannot identify the fault or establish the cause of the recovery requirement. **Do not repeat the device test or install 0.2.** Prior controlled-device-test instructions are withdrawn.

Replacement delivery is held by `ci/HARDWARE-HOLD`. Debug symbols and original packages are preserved for investigation. The source review and limitations are recorded in [startup audit](STARTUP-AUDIT.md). No replacement is described as safe or hardware accepted.
