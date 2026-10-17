# Forum draft — prepared, not posted

Suggested board: [Subset Games → Mod Development](https://www.subsetgames.com/forum/viewforum.php?f=12), since this is an initial beta needing broader testing. Follow the board's current posting rules. Move/repost to Working Mods when appropriate.

**Topic title:** `[BETA][Windows] FTL Ultrawide - 3440x1440 (43:18)`

Copy the BBCode below into a new topic. Review the formatted preview before submitting.

```bbcode
[size=150][b]FTL Ultrawide — Windows beta[/b][/size]

I've been working on a presentation mod for base-game FTL with Advanced Edition, targeting my 3440x1440 ultrawide display. It expands the game scene and places the HUD at the sides, with wider scenery and adapted menus/effects.

[b]Download / source / instructions[/b]
[url=https://github.com/Gavitron2021/ftl-ultrawide/releases/tag/v0.1.0-beta.2]Beta download and SHA-256 checksum[/url]
[url=https://github.com/Gavitron2021/ftl-ultrawide]Public source repository[/url]
[url=https://github.com/Gavitron2021/ftl-ultrawide/blob/main/docs/INSTALL.md]Installation, saves and rollback[/url]
[url=https://github.com/Gavitron2021/ftl-ultrawide/blob/main/docs/COMPATIBILITY.md]Exact display support and test scope[/url]

[b]What it does[/b]
[list]
[*]Adds Fullscreen (Modded), alongside Windowed and Fullscreen (Classic). Classic retains the original aspect ratio with side bars; restart after display-mode changes.
[*]Expands the viewport and moves the combat HUD to the edges; adjusts menus, pause/modal darkening, Advanced Information and the startup loading animation.
[*]Adds 31 complete gameplay panoramas. Title and hangar retain their original center art with repaired extensions, including paths followed by the hangar worker.
[*]Upgrades four dynamic nebula/storm textures while preserving native opacity and lightning intensity; widens environment layers/flashes and fixes Credits cropping.
[*]Uses a compatible UI-only selection from Shiny, plus HighRes planets and Better Asteroids. Original ship, crew and weapon sprites are preserved.
[/list]

[b]Requirements[/b]
Windows Steam FTL, a pristine hash-verified game archive, the verified Steam 1.6.9 executable, Hyperspace 1.23.2, ftlman 0.7.4, Python 3.11+ and Pillow. The builder can roll back the tested Steam 1.6.14 executable with the separately downloaded official patch. Other starting versions require official rollback first; unknown files are rejected.

Also obtain the exact author downloads for HighRes Backgrounds & Planets 2.0, Better Asteroids, Shiny UI and Graphics v1.0 Vanilla and Shiny Combat UI 2.0. The install guide links them and lists required files. They are separate dependencies, not rehosted in this package. Our builder applies the selected resources and patches locally; this is more involved than installing a single .ftl file.

[b]Monitor support — please read[/b]
Only 3440x1440 has been tested on hardware. The mod uses a 1720x720 internal canvas, scaled 2x on that display. Higher resolution at the same exact 43:18 ratio (e.g. 6880x2880) should scale uniformly, but is untested and adds no extra sprite detail.

The label "21:9" covers different ratios. 2560x1080, 3840x1600 and 5120x2160 are not the same ratio as 3440x1440; this beta deliberately rejects Modded mode when the calculated logical width differs from 1720. 32:9 is unsupported. Use Classic on other ratios. The primary Windows display determines the startup layout.

[b]Saves and limitations[/b]
The installer backs up affected files and existing saves without replacing the saves. On first creation, the production Hyperspace profile imports vanilla ships, achievements and statistics from ae_prof.sav into hs_ultrawide_prof.sav. Existing mod progress is reused; new progress does not automatically sync back to vanilla. Active vanilla Continue migration is not promised. Check Stats and ship unlocks after first launch.

Selected menus, shops, combat/events, nebula/storm effects, Credits, startup, Steam launching and profile import have been checked. A complete campaign has NOT been validated. Multiverse, Captain's Edition, other overhauls, other operating systems and localization layouts are unvalidated. Hyperspace itself changes engine behavior; exact vanilla gameplay equivalence is not claimed. Steam updates/Verify Integrity can replace the installed mod.

[b]Artwork disclosure and thanks[/b]
The 31 gameplay panoramas are ImageGen-generated complete frames, based on existing scenery for style and encounter context, then curated and reviewed. Title/hangar extensions and cloud detail combine generation with conventional alignment, sampling, compositing and color matching. The original title/hangar center art remains in place. Lightning is conventionally upscaled to preserve its effect.

Thanks to Subset Games, the Hyperspace team, afishhh/ftlman, Ouaz, Russian Rockman, Sanmonku, Splette, Scooterbaga and the other upstream contributors. Their original art/mod downloads remain separate; no blanket ownership or redistribution rights over their work are claimed. Full provenance, component links and local DLL modifications are documented in the repository's CREDITS.md.

[b]Screenshots[/b]
[img]https://raw.githubusercontent.com/Gavitron2021/ftl-ultrawide/main/docs/screenshots/title.png[/img]
[img]https://raw.githubusercontent.com/Gavitron2021/ftl-ultrawide/main/docs/screenshots/plasma-storm.png[/img]

[b]Testing feedback welcome[/b]
Please report reproducible problems with resolution, Windows scaling, GPU, display mode, low-end graphics setting, exact versions, screenshot and reproduction steps. Full-campaign feedback is particularly useful. Please don't upload your full game or save folder.

[url=https://github.com/Gavitron2021/ftl-ultrawide/issues/new/choose]Report an issue[/url]
```
