# Credits and rights

FTL: Faster Than Light belongs to **Subset Games**. This is an unofficial fan mod; no affiliation or endorsement is claimed. Original ultrawide code/documentation by **Gavitron2021**, developed with Codex, is under the scoped MIT license. That license does not cover artwork or other authors' material.

| Component | Authors / source | Treatment |
| --- | --- | --- |
| Hyperspace **1.23.2** | [Hyperspace team](https://github.com/FTL-Hyperspace/FTL-Hyperspace/releases/tag/v1.23.2), [tag-specific license/notices](https://github.com/FTL-Hyperspace/FTL-Hyperspace/blob/v1.23.2/LICENSE.md) | Separate official download, patched locally. This tag states CC BY-SA 4.0 with additional component notices; current master can differ. No DLL/full default configuration bundled. |
| FTL-Version-Rollback | [Official rollback project](https://github.com/FTL-Hyperspace/FTL-Version-Rollback) | Separate patch download; no FTL executable bundled. Our BPS decoder follows byuu's [public-domain specification](https://github.com/Alcaro/Flips/blob/master/bps_spec.md). |
| ftlman **0.7.4** | [afishhh and contributors](https://github.com/afishhh/ftlman/releases/tag/v0.7.4) | Separate download, operates on private build directory. |
| HighRes Backgrounds & Planets **2.0** | [Ouaz and upstream artists](https://github.com/Ouaz/FTL-diversityMod#highres-backgrounds--planets-v20), [forum](https://www.subsetgames.com/forum/viewtopic.php?f=11&t=30003) | Separate download; title/cloud sources and locally installed planets/image lists. |
| Better Asteroids | **sanmonku**, as credited in its readme; [standalone author download](https://www.moddb.com/mods/better-planets-and-backgrounds/downloads/better-asteroids) | Separate download; asteroid resources and lightning patch input. |
| Shiny! UI and Graphics **v1.0 Vanilla** | **Russian Rockman**, [author thread](https://www.subsetgames.com/forum/viewtopic.php?f=11&t=25839) | Only the 794 compatible UI resource paths are extracted locally; excludes its ship/crew/weapon/scenery overrides. |
| Shiny Combat UI **2.0** | **Ouaz**, based on Russian Rockman's UI, [author description](https://github.com/Ouaz/FTL-diversityMod#shiny-combat-ui-20) | Separate author download. |
| 31 new gameplay panoramas | ImageGen output curated/reviewed for this project | Complete generated frames; existing scenery supplied style and encounter context. AI use does not establish exclusive rights or clear reference rights. |
| Title/hangar additions and environment repairs | Generated detail and conventional source-based sampling/compositing | Source-dependent local texture deltas. Original centers/full original textures are not bundled. Derivative modifications do not grant rights over the source artwork. |

The HighRes project credits **Splette, Sanmonku, Scooterbaga, Russian Rockman, Unnamed and NASA** among its contributors. Retain the original notices when using the separately downloaded mods. **Attribution is not permission.** No blanket third-party artwork redistribution permission has been verified or claimed. We do not mirror the original mod downloads or license them under MIT. Ask the relevant authors before rehosting their work or reusing derived art outside this FTL mod. Rights holders can open an issue for review of an affected patch/asset.

## Local engine changes

`release/hyperspace-patches.json` publishes the hash-guarded recipe for the official 1.23.2 Windows DLL: correct the obsolete OpenGL-flag substring check; route cross-CRT `ftl_log` calls to the DLL-owned `hs_log_file` while retaining diagnostics; adjust only the six mouse-offset bytes in two CApp signatures. At launch the subsystem and Advanced Information anchors move with the wide canvas.

The owned FTL 1.6.9 executable is verified then display-patched **in the suspended child process**, remaining unchanged on disk. Our launcher replaces Steam's entry executable and waits for the real game to exit.

## Art provenance

`release/assets/scenes.json` records scene mappings/hashes. `release/assets/texture-patches.json` records six source-dependent texture repairs, original-source hashes, reconstruction methods and approved output pixel hashes. Three cloud textures retain native broad color/opacity with generated fine detail; lightning retains the original silhouette/intensity. Title cleanup also repairs one-pixel edge fringes. The hangar retains its center and samples native floor/path detail in the additions. The texture deltas and artwork are excluded from the MIT code license; upstream rights remain with their owners.
