# Coexistence and hardware acceptance

There is currently **no verified installable Plexium native binary**. Do not copy host executables or metadata-only folders to the PS5. Firmware 13.40, kstuff Lite and ShadowMountPlus compatibility is pending a real native build and device testing.

After an actual package passes `tools/check-package.py` and the native tool's SELF inspection:

1. Keep PlexStation's `PPSA85301` installation, account, caches and matching debug files intact. Back up both app folders/data and your launcher configuration before changing console files. Do not migrate PlexStation data into Plexium.
2. Read-only inspect installed titles and ShadowMount discovery. Verify `PPSA85421` is unused throughout mounted/staged/registered titles, not merely absent from one folder. Global allocation was not checked. If occupied, stop; choose and propagate another candidate in a separate reviewed change.
3. Close relevant applications. Copy the complete new `PPSA85421` folder to `/data/homebrew/PPSA85421` using the user's existing supported ShadowMountPlus procedure. Keep all imports, modules, fonts, CA bundle, notices and metadata together. Preserve 755 directory/executable/module and 644 public-file modes. Private runtime storage must remain 0700/0600; never apply recursive 777.
4. Plexium uses its own `/data/plexium-client` private account/cache root, and title-local prefs under its independent mounted directory. Verify this is writable in the actual console sandbox. Do not relax private storage checks if inaccessible; implement a supported isolated writable path after diagnosis.
5. First launch must show Plexium and accept DualSense Cross/Circle/D-pad/L1/R1. Then verify actual device linking, managed PINs, server identity, denied libraries, initial rendering without button presses, shows/seasons/episodes and the episode's own playback identity.
6. Test real video **and audible audio**, pause/seek/resume, track and subtitle switching, progress sync, app return, long sessions and resource cleanup. Retain the exact matching native ELF/maps and package checksum. Compilation is not device acceptance.

## Read-only artwork diagnosis

Use `tools/Inspect-PlexiumArtwork.ps1` on Windows with a verified package and read-only downloaded mirrors of mounted/staged/registered artwork. Example:

```powershell
./tools/Inspect-PlexiumArtwork.ps1 -PackageRoot C:\Plexium\PPSA85421 -MountedMirror C:\ReadOnlyMirrors\Mounted
```

It compares hashes and reports missing stages. It does not connect to the console, discover registration paths, query the app database, modify cache or uninstall titles. Its console/PowerShell execution is untested here. Verify each real path through your existing launcher diagnostic workflow; never guess a registration DB location. Check packaged icon, mounted icon, staged icon, discovered title ID, registered artwork, duplicate registrations, launcher artwork references and finally the shell's visible icon. Capture the actual console screen. A generated icon is not proof of registration.

## Separately initiated refresh and rollback

If all read-only hashes agree and the shell is stale, preserve backups, close Plexium and use only the launcher's documented title-specific refresh/discovery operation. A normal supported launcher/console restart may be needed. No automatic uninstall, registration database manipulation or system-wide cache deletion is provided. If the launcher has no documented safe title-specific refresh, stop and collect diagnostics. Roll back only the new Plexium folder through the user's supported procedure after closing/unmounting it; retain its private data until the user decides otherwise. PlexStation remains untouched as the fallback.
