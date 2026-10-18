# Initial beta validation

Packaging validation performed 2026-10-17. Gameplay observations are from the preceding private build on Windows at 3440×1440; the public launcher adds an aspect-ratio guard without changing its supported-resolution layout.

| Check | Result |
| --- | --- |
| Local rebuild from pristine owned Steam 1.6.14 inputs | Passed; official rollback produces the exact supported Steam 1.6.9 SHA-256 |
| 31 gameplay panoramas and their native center crops | All approved panorama pixels and exact crop continuity verified |
| Six source-dependent texture patches | Title, hangar, three cloud textures and lightning exactly match approved output pixels |
| Original ship, crew and weapon resources | 651 PNG resources byte-identical to pristine input |
| Production profile configuration | `ultrawide`, inheritance mode `0`, developer console disabled |
| In-game artwork review/debug display | F9 gallery/hint and artwork filename logger absent |
| Hyperspace modifications | Exact original and patched DLL SHA-256 checks pass |
| Installer tests | 10 passed: rollback decoding, corruption/bounds checks, payload validation, interrupted-copy rollback, backup verification, path containment, original/new save preservation, changed-file protection and settings handling |
| Disposable install/restore with real owned game files | Original files restored; unrelated files and original/new synthetic profiles preserved; current Steam installation untouched |
| Native launcher | MSVC x86 build succeeds |

Previously checked live: startup/loading, Steam entry behavior, selected menus/shops/events/combat, pause and modal darkening, hangar/ship list, nebula/storm effects, hazard flashes, Credits and original profile unlock/statistics import. Screenshots show the verified private build's title and plasma-storm presentation.

No uninterrupted full campaign, other display hardware, localized layout, overhaul compatibility or higher-resolution same-ratio run has been validated. These remain beta test goals; passing reconstruction/installer checks does not establish them.

## Beta.4 follow-up, 2026-10-18

The MSVC x86 build and all 13 installer/update tests pass. The pristine local build verifies 17 native UI fallbacks, 31 panorama/crop pairs, six source-dependent texture reconstructions and 651 unchanged base sprite PNGs. Disposable UI checks cover reactor normal/hover/queued/MAX states, purchase costs across a column boundary, Undo refunds, and store Buy/Sell Close frames and clicks at Modded 3440x1440 and Windowed 1280x720. See [UI causes and scope](BUGFIX-NOTES.md).
