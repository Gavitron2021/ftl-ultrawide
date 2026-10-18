# Compatibility

The launcher reads the **primary Windows desktop resolution**, then calculates `round(720 × desktop_width / desktop_height)`. Fullscreen (Modded) and Borderless (Modded) in this beta require a logical width of **1720**: the approved art and hangar route were built for precisely 1720×720. Set the target monitor as primary before launching. Moving monitors or changing the desktop resolution while running is unvalidated; restart after changes.

| Desktop resolution | Logical width | Modded mode | Evidence |
| --- | ---: | --- | --- |
| 3440×1440 | 1720 | Accepted | Hardware tested, 2× scale |
| 5160×2160 | 1720 | Accepted | Exact same ratio, 3× scale; untested |
| 6880×2880 | 1720 | Accepted | Exact same ratio, 4× scale; untested |
| 2560×1080 | 1707 | Rejected | Different ratio; use Classic |
| 5120×2160 | 1707 | Rejected | Different ratio; use Classic |
| 3840×1600 | 1728 | Rejected | Different ratio; use Classic |
| 5120×1440 | 2560 | Rejected | 32:9; needs more scenery/layout work |
| 1920×1080 / 3840×2160 | 1280 | Rejected | 16:9; use Classic/Windowed |

Higher resolution at the same ratio means **uniform scaling**, not horizontal distortion of a 16:9 frame. The working canvas remains 1720×720; no additional sprite detail is created. Non-integer scaling can soften pixel art. DPI scaling, HDR, refresh rates, high-resolution GPU limits and mixed-monitor scaling have not been systematically tested. Classic/Windowed are fallback choices, not claims of additional hardware validation.

## Software

- Windows Steam base FTL with Advanced Edition, verified Steam **1.6.9** executable and pristine archive. Automatic rollback accepts only the tested **1.6.14** source. Officially rolled-back, hash-matching Steam 1.6.9 is accepted too.
- Exactly **Hyperspace 1.23.2** and **ftlman 0.7.4**; hashes are in `release/dependencies.json`. Different executable/DLL versions are refused. The binary hooks are version-specific; changing hash checks alone does not port the mod.
- Explicit **OpenGL** renderer. Direct3D is not supported by this launcher.
- Multiverse, Captain's Edition, other overhauls/mod combinations, other storefronts, Linux/SteamOS, macOS and localization layouts are unvalidated. Full Shiny UI is not installed: a compatible UI-only selection is used.
- Presentation changes are intended; Hyperspace has engine fixes/features of its own. Configurable behavior was reduced toward vanilla, but exact vanilla equivalence is not guaranteed.

## Test scope

Beta.5 adds the Borderless option at the same supported ratio. It uses the Modded renderer; desktop size/refresh and focus switching were checked at 3440x1440. See [Borderless implementation and limits](BORDERLESS.md).

Beta.4 additionally checks hidden reactor column labels, the 8-to-11 capacity price boundary (65 scrap), complete Undo refund, final 24-to-25 purchase and MAX, plus store Buy pages and Sell-tab Close hover/click. Modded 3440x1440 and Windowed 1280x720 were checked; outside/inside Close-edge clicks were checked in Windowed mode. The Classic layout shares the 1280-wide renderer but was not replayed for beta.4.

Beta.3 adds isolated verification of right-edge tooltips/clicks, reactor costs/capacity and queued/MAX states, ship Accept hover/click, a centered store, sensor reveal and two enemy weapon layouts, plus Classic/Windowed restarts. The installed production update is also checked through the actual Steam entry point with current progress retained. [UI causes and detailed scope](BUGFIX-NOTES.md).

At 3440×1440: startup/loading, normal Steam entry behavior, main menu/Stats, title/hangar/ship list, selected shop pages and equipment displays, pause/modal darkening, selected combat/events, nebula/storm presentation including plasma storms, hazard flashes, Credits, normal shutdown and original vanilla profile import. Base ship/crew/weapon resource integrity was checked.

**Not completed:** uninterrupted campaign through the flagship, every event/store transaction, every ship/layout, all languages, all GPUs/display modes, mixed-monitor/DPI combinations and overhaul compatibility. Public packaging adds a ratio guard and portable builder/installer; disposable-directory installation tests do not constitute a complete gameplay run.
