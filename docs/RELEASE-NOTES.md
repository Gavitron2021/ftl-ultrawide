# FTL Ultrawide 0.1.0-beta.4

Bug-fix update for the **Windows Steam base-game Advanced Edition** beta at **3440×1440 (43:18)**. Hides the reactor prices above each column while keeping capacity, next cost and MAX visible. Restores the native store Close frame to match its hover/click area, extending the ship-menu Accept fix. Includes the prior tooltip, charge-indicator and upgrade lettering fixes. Reactor upgrade prices and gameplay calculations are unchanged. Approved artwork and base gameplay sprites are retained.

Existing users can use the new [guarded update route](https://github.com/Gavitron2021/ftl-ultrawide/blob/main/docs/INSTALL.md#update-an-existing-modded-installation): rebuild from pristine owned inputs, inventory the current modded copy, then update with current-file/save backups and settings preservation. Keep the earlier vanilla rollback backup. Do not rerun the fresh installer against a modded archive.

**Download `FTL-Ultrawide-0.1.0-beta.4.zip`**, not GitHub's automatic source ZIP if you want the compiled launcher. Check `SHA256SUMS.txt`, extract outside FTL, then follow [INSTALL.md](https://github.com/Gavitron2021/ftl-ultrawide/blob/main/docs/INSTALL.md). Python 3.11+ and Pillow are required. Source/build instructions and installer tests are included.

Current setup help: [visual mod download links](https://github.com/Gavitron2021/ftl-ultrawide#required-visual-mod-downloads) and [copyable agent / LLM preparation and installation prompts](https://github.com/Gavitron2021/ftl-ultrawide/blob/main/docs/AGENT-SETUP.md). These online guides supplement the documentation originally packaged in the beta ZIP.

Prerequisites: owned pristine Steam FTL archive; verified Steam FTL **1.6.9** executable (builder can roll back the tested **1.6.14** source); exact **Hyperspace 1.23.2**, **ftlman 0.7.4**, HighRes Backgrounds & Planets 2.0, Better Asteroids, Shiny UI/Graphics v1.0 Vanilla and Shiny Combat UI 2.0 downloads. These dependencies/game files are **not bundled**. The package reconstructs them locally and includes backed-up installation/rollback. Do not redistribute generated local payloads.

Only **3440×1440** has hardware testing. A higher resolution at the exact same ratio should scale uniformly but is untested. Other common “21:9” ratios and 32:9 are deliberately refused by Modded mode; Classic remains available. [Full compatibility table](https://github.com/Gavitron2021/ftl-ultrawide/blob/main/docs/COMPATIBILITY.md).

The original vanilla profile is preserved, and ships/statistics are imported into `hs_ultrawide` when first created. No continuous vanilla synchronization or active Continue migration is promised. Check progress after first launch. No full-campaign validation, overhaul/Multiverse support, other OS support or localization-layout guarantee.

**AI artwork disclosure:** 31 complete ImageGen gameplay scenes, human curated/reviewed; title/hangar original centers retained with generated/conventional additions; native cloud color/opacity and lightning intensity preserved. [Credits and rights](https://github.com/Gavitron2021/ftl-ultrawide/blob/main/CREDITS.md). Original code is MIT; that license excludes artwork and upstream material.
