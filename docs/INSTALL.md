# Install and restore

This beta targets Windows Steam base FTL Advanced Edition at **3440×1440's aspect ratio**. [Read compatibility limits](COMPATIBILITY.md). Only 3440×1440 has hardware testing.

If an agent or LLM is helping, use the **[copyable setup and installation prompts](AGENT-SETUP.md)**. They reference this guide and the shipped tools; preparation creates a local build before installation changes your game folder.

## Downloads

Extract **FTL-Ultrawide-0.1.0-beta.6.1.zip** from [Releases](https://github.com/Gavitron2021/ftl-ultrawide/releases/tag/v0.1.0-beta.6.1) outside your game folder. It includes our launcher. A source checkout instead needs [the launcher build](BUILD.md).

Obtain these exact files from their authors and place/extract them under one dependency directory, e.g. `C:\FTL-Mod-Dependencies`. Subdirectories are searched; all inputs are SHA-256 checked against [`release/dependencies.json`](../release/dependencies.json).

| Dependency | Author download | Files required |
| --- | --- | --- |
| Hyperspace **1.23.2 Windows** | [Tagged release](https://github.com/FTL-Hyperspace/FTL-Hyperspace/releases/tag/v1.23.2) | Extract `Hyperspace.ftl`, original `Hyperspace.dll`, `xinput1_4.dll` |
| ftlman **0.7.4 Windows** | [Tagged release](https://github.com/afishhh/ftlman/releases/tag/v0.7.4) | Extract its folder containing `ftlman.exe` |
| Official Steam rollback patch | [FTL-Version-Rollback](https://github.com/FTL-Hyperspace/FTL-Version-Rollback) | `Steam-1.6.14.bps`, required by this pinned set even if already rolled back |
| HighRes Backgrounds & Planets **2.0** | [Download .ftl](https://github.com/Ouaz/FTL-diversityMod/raw/master/other/HighRes_Backgrounds_and_Planets_2.0.ftl), [Ouaz's project](https://github.com/Ouaz/FTL-diversityMod#highres-backgrounds--planets-v20), [thread](https://www.subsetgames.com/forum/viewtopic.php?f=11&t=30003) | `HighRes_Backgrounds_and_Planets_2.0.ftl` |
| Better Asteroids | [Standalone author download on ModDB](https://www.moddb.com/mods/better-planets-and-backgrounds/downloads/better-asteroids) | Extract ZIP to get `better_asteroids.ftl` |
| Shiny! UI and Graphics **v1.0 Vanilla** | [Russian Rockman's thread and downloads](https://www.subsetgames.com/forum/viewtopic.php?f=11&t=25839) — choose **Vanilla v1.0** | `Shiny_UI_and_Graphics_v1.0_Vanilla.ftl` |
| Shiny Combat UI **2.0** | [Download .ftl](https://github.com/Ouaz/FTL-diversityMod/raw/master/other/Shiny_Combat_UI_2.0.ftl), [Ouaz's project](https://github.com/Ouaz/FTL-diversityMod#shiny-combat-ui-20) | `Shiny_Combat_UI_2.0.ftl` |

If a download changes or disappears, open an issue. Do not substitute DLLs or bypass the hash checks. We do not mirror the original visual mods. [Credits and provenance](../CREDITS.md).

All four visual archives are required by the current builder. Keep `.ftl` files intact; extract outer download ZIPs only as needed to expose the listed files. **Do not first apply these mods to your live game with another mod manager.** Supply them as inputs: our builder selects compatible Shiny UI assets and applies the tested load order in the private build directory.

Install **Python 3.11+**, then from the extracted mod directory:

```powershell
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r .\release\requirements.txt
```

## Prepare FTL

Close FTL. Use a pristine Steam `ftl.dat`, not an already modded archive. Keep a separate save backup and review Steam Cloud/update settings before switching setups. [Official Hyperspace guidance](https://ftl-hyperspace.github.io/FTL-Hyperspace/en/windows/manual-install/) describes rollback and update/cloud behavior. **Do not delete/reset your profile to install this mod.** Our installer preserves the original and backs it up.

Automatic rollback accepts the tested **Steam 1.6.14 executable**; the verified **Steam 1.6.9 executable** is also accepted. Other starting versions need the official rollback route first. Unknown executable/archive hashes are rejected. Compatibility with arbitrary newer Steam downloads is not claimed.

Set the target monitor as primary at a supported ratio. Substitute your own paths below; no Steam drive is hard-coded.

## Build and install

```powershell
.\.venv\Scripts\python.exe .\release\ftl_ultrawide.py build --game-dir "C:\Program Files (x86)\Steam\steamapps\common\FTL Faster Than Light" --dependencies "C:\FTL-Mod-Dependencies" --out "C:\FTL-Ultrawide-Local-Build"
```

The output path must not exist and must be outside the game folder. This command reads owned files and creates a **private local payload** without changing the installed game/saves or launching FTL. ftlman receives only the disposable build directory. Do not share this output: it contains reconstructed game files/upstream resources.

With FTL closed:

```powershell
.\.venv\Scripts\python.exe .\release\ftl_ultrawide.py install --game-dir "C:\Program Files (x86)\Steam\steamapps\common\FTL Faster Than Light" --build "C:\FTL-Ultrawide-Local-Build"
```

Installation verifies the payload and unchanged game baseline, checks affected files are not in use, backs up overwritten files and existing `.sav` files, then copies ten payload files. The timestamped **`ftl-ultrawide-backup-…` folder inside FTL's folder** contains hashes and originals; keep it. Copy failures trigger a rollback attempt.

For NVIDIA capture stutter or long black app-switch transitions, append `--nvidia-dxgi` to the install/update command. This explicitly creates a reversible profile for this installation only, with **Prefer layered on DXGI Swapchain**. Existing user profiles are refused. No global settings change. Keep the printed backup: its restore command also removes the unchanged profile it created. See [evidence, scope and rollback details](BORDERLESS.md#nvidia-capture-and-app-switching). Omit this option on other GPUs.

Windows' Documents known folder, including redirected Documents, is resolved automatically. Optional `--save-dir` supplies a nonstandard save location. Write access to the game folder is needed; the installer does not impose a general administrator requirement.

Launch with **Steam Play**. Modded mode is initially selected. Options cycles Windowed / Fullscreen (Classic) / Fullscreen (Modded) / Borderless (Modded); **restart to apply**. Borderless uses the same supported ultrawide layout at the exact primary desktop size and stays open when switching apps; select it and restart. Classic preserves the original aspect ratio with side bars. See [Borderless details](BORDERLESS.md). To recover from an unsupported-ratio error, run `FTL-Ultrawide.exe --classic` from the game folder.

## Update an existing modded installation

Close FTL. Build the new version from a **separate verified pristine game backup**, using the build command above with that backup as `--game-dir`. Never build from the live modded archive. Keep the original vanilla rollback backup.

Record the current installation for review, then update from the new private build:

```powershell
.\.venv\Scripts\python.exe .\release\ftl_ultrawide.py inventory --game-dir "C:\Games\FTL" --out "C:\FTL-current-files.json"
.\.venv\Scripts\python.exe .\release\ftl_ultrawide.py update --game-dir "C:\Games\FTL" --build "C:\FTL-Ultrawide-Local-Build" --expected-current "C:\FTL-current-files.json"
```

Substitute your actual game/build paths. Review the inventory's game directory and file hashes before updating. The command refuses any change since that inventory, requires the pinned native executable/DLL baseline and production `ultrawide` / `inheritMode=0` configuration, and validates every new payload hash. It backs up **current** modded files and all current saves, then copies only changed production files. Settings, display mode and generated runtime files are preserved; the launcher regenerates the active Hyperspace DLL at the next launch. No previous save/profile is imported over existing progress.

Use the printed `restore --backup` command to roll back this update to the immediately preceding modded installation. It never restores saves. The earlier vanilla backup remains the route back to vanilla. Local inventories/builds/backups contain private paths or owned files; do not upload them.

## Saves

Check Stats and ship unlocks immediately after first launch. Original **`ae_prof.sav` remains unchanged**. Hyperspace imports its ships, achievements and statistics into **`hs_ultrawide_prof.sav`** when the production profile is first created. An existing production profile is reused, not continuously overwritten from vanilla. New progress does not automatically sync back to vanilla. Migration of an active vanilla Continue run is not guaranteed.

If progress looks wrong, close FTL and preserve all profiles plus the installation backup. Do not delete saves. Report symptoms/versions and selected sanitized logs rather than your whole save folder. Steam Cloud can affect synchronization; keep independent backups when switching setups.

## Restore / uninstall

Close FTL and use the exact backup path printed by installation:

```powershell
.\.venv\Scripts\python.exe .\release\ftl_ultrawide.py restore --backup "C:\Program Files (x86)\Steam\steamapps\common\FTL Faster Than Light\ftl-ultrawide-backup-YOUR-TIMESTAMP"
```

Restoration verifies backup hashes, restores replaced files and removes only recorded additions. **It never restores/overwrites/deletes saves.** Backup and normal diagnostic logs remain. Unexpectedly changed added files are refused for inspection; settings, the generated active Hyperspace DLL and runtime configuration are expected to change during play.

Restore before returning to vanilla or using Steam Verify Integrity. Updates/Verify can replace the launcher/archive. Use the guarded update route above for a version update, or restore before a fresh installation; do not layer fresh installations.

## Troubleshooting

- **Ratio rejected:** use Classic and review the exact-ratio table; “21:9” alone is insufficient.
- **Hash mismatch:** wrong version, already modified files or changed author download. Use pinned pristine inputs; do not force past the checks.
- **File in use / cannot prepare Hyperspace:** close every FTL instance.
- **No PIL module:** use the virtual environment's Python after installing `release/requirements.txt`.
- **Visual/input bug:** report resolution, Windows scaling, GPU, display mode, low-end graphics setting, versions, screenshot and reproduction steps. Full-campaign testing is still needed.
