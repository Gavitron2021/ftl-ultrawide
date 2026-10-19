# FTL Ultrawide 0.1.0-beta.6.1

Corrects **Borderless (Modded)** window/focus behavior and adds optional,
reversible NVIDIA DXGI presentation for capture stability and smoother app
switching. Select Borderless in Options and restart. On the tested RTX 5080 host,
Discord-sharing GPU allocation stayed near 235–261 MB for six minutes after
previously growing to about 8 GB; the tester confirmed smooth streaming and HDR.
Rapid switching/opening apps can still stutter. [Setup and scope](https://github.com/Gavitron2021/ftl-ultrawide/blob/main/docs/BORDERLESS.md).

For this NVIDIA workaround, append **`--nvidia-dxgi`** to install/update. It creates
one installation-specific driver profile and records guarded rollback; global
settings and existing user profiles are not modified. The flag is explicit and
should be omitted on other GPUs. Keep the timestamped installation backup.

The initial beta.6 tag exposed a Windows short-TEMP-path issue in the test fixture.
Beta.6.1 corrects the fixture; the tested native binary is unchanged.

Fresh Start, first event, HUD/ship interaction, normal Save+Quit and Continue
passed in all four offered modes. Native compilation, local reconstruction/archive
guards and 22 installer/profile rollback tests passed. The earlier immediate
run-entry access violation remains unproven and did not reproduce in acceptance.
This remains a community-test beta on one hardware configuration, with the
**60 FPS limit unchanged**. [Investigation details](https://github.com/Gavitron2021/ftl-ultrawide/blob/main/docs/BUGFIX-NOTES.md).

Existing users can use the new [guarded update route](https://github.com/Gavitron2021/ftl-ultrawide/blob/main/docs/INSTALL.md#update-an-existing-modded-installation): rebuild from pristine owned inputs, inventory the current modded copy, then update with current-file/save backups and settings preservation. Keep the earlier vanilla rollback backup. Do not rerun the fresh installer against a modded archive.

**Download `FTL-Ultrawide-0.1.0-beta.6.1.zip`**, not GitHub's automatic source ZIP if you want the compiled launcher. Check `SHA256SUMS.txt`, extract outside FTL, then follow [INSTALL.md](https://github.com/Gavitron2021/ftl-ultrawide/blob/main/docs/INSTALL.md). Python 3.11+ and Pillow are required. Source/build instructions and installer tests are included.

Current setup help: [visual mod download links](https://github.com/Gavitron2021/ftl-ultrawide#required-visual-mod-downloads) and [copyable agent / LLM preparation and installation prompts](https://github.com/Gavitron2021/ftl-ultrawide/blob/main/docs/AGENT-SETUP.md). These online guides supplement the documentation originally packaged in the beta ZIP.

Prerequisites: owned pristine Steam FTL archive; verified Steam FTL **1.6.9** executable (builder can roll back the tested **1.6.14** source); exact **Hyperspace 1.23.2**, **ftlman 0.7.4**, HighRes Backgrounds & Planets 2.0, Better Asteroids, Shiny UI/Graphics v1.0 Vanilla and Shiny Combat UI 2.0 downloads. These dependencies/game files are **not bundled**. The package reconstructs them locally and includes backed-up installation/rollback. Do not redistribute generated local payloads.

Only **3440×1440** has hardware testing. A higher resolution at the exact same ratio should scale uniformly but is untested. Other common “21:9” ratios and 32:9 are deliberately refused by Modded mode; Classic remains available. [Full compatibility table](https://github.com/Gavitron2021/ftl-ultrawide/blob/main/docs/COMPATIBILITY.md).

The original vanilla profile is preserved, and ships/statistics are imported into `hs_ultrawide` when first created. No continuous vanilla synchronization or active Continue migration is promised. Check progress after first launch. No full-campaign validation, overhaul/Multiverse support, other OS support or localization-layout guarantee.

**AI artwork disclosure:** 31 complete ImageGen gameplay scenes, human curated/reviewed; title/hangar original centers retained with generated/conventional additions; native cloud color/opacity and lightning intensity preserved. [Credits and rights](https://github.com/Gavitron2021/ftl-ultrawide/blob/main/CREDITS.md). Original code is MIT; that license excludes artwork and upstream material.
