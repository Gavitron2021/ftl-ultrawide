# FTL Ultrawide 0.1.0-beta.2

Initial **Windows Steam base-game Advanced Edition** beta for **3440×1440 (43:18)**. Adds Modded fullscreen, side-aligned HUD, adapted menus/effects, 31 complete gameplay panoramas, repaired title/hangar additions and upgraded dynamic nebula/storm textures. Original ship/crew/weapon sprites are retained.

**Download `FTL-Ultrawide-0.1.0-beta.2.zip`**, not GitHub's automatic source ZIP if you want the compiled launcher. Check `SHA256SUMS.txt`, extract outside FTL, then follow [INSTALL.md](https://github.com/Gavitron2021/ftl-ultrawide/blob/main/docs/INSTALL.md). Python 3.11+ and Pillow are required. Source/build instructions and installer tests are included.

Prerequisites: owned pristine Steam FTL archive; verified Steam FTL **1.6.9** executable (builder can roll back the tested **1.6.14** source); exact **Hyperspace 1.23.2**, **ftlman 0.7.4**, HighRes Backgrounds & Planets 2.0, Better Asteroids, Shiny UI/Graphics v1.0 Vanilla and Shiny Combat UI 2.0 downloads. These dependencies/game files are **not bundled**. The package reconstructs them locally and includes backed-up installation/rollback. Do not redistribute generated local payloads.

Only **3440×1440** has hardware testing. A higher resolution at the exact same ratio should scale uniformly but is untested. Other common “21:9” ratios and 32:9 are deliberately refused by Modded mode; Classic remains available. [Full compatibility table](https://github.com/Gavitron2021/ftl-ultrawide/blob/main/docs/COMPATIBILITY.md).

The original vanilla profile is preserved, and ships/statistics are imported into `hs_ultrawide` when first created. No continuous vanilla synchronization or active Continue migration is promised. Check progress after first launch. No full-campaign validation, overhaul/Multiverse support, other OS support or localization-layout guarantee.

**AI artwork disclosure:** 31 complete ImageGen gameplay scenes, human curated/reviewed; title/hangar original centers retained with generated/conventional additions; native cloud color/opacity and lightning intensity preserved. [Credits and rights](https://github.com/Gavitron2021/ftl-ultrawide/blob/main/CREDITS.md). Original code is MIT; that license excludes artwork and upstream material.
