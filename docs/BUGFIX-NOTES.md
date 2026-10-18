# Beta.3 UI corrections

The supported canvas remains 1720×720, scaled 2× at 3440×1440. Its added width is 440 logical pixels, with 220 logical pixels of centered menu padding per side. These are different transforms.

## Tooltips

Hyperspace 1.23.2 replaces `MouseControl::RenderTooltip` and retains the old 1260/1240 horizontal edges. The moved controls already use correct mouse/hit regions; the renderer was pushing their tips back toward the old edge.

The launcher supplies private bounds of `width - translation - 20` and `-(width - translation - 40)` to the two guarded horizontal-bound operands. The current outer translation is zero in ordinary combat and 220 for centered menus/focus windows. Native tooltip measurement, vertical clipping/scrolling, mouse coordinates and control hitboxes remain in use. Classic and Windowed use the original DLL bounds.

## Upgrade and reactor text

Older Shiny system/subsystem textures bake MAX lettering into the cost strip; reactor textures also bake POWER BARS/MAX. Current FTL/Hyperspace renders these labels dynamically. The final local overlay restores the owned native textures for those families. The rest of the selected UI remains enabled.

The small numbers above the reactor columns are **intentional Hyperspace prices per upgrade in each five-level group**, not duplicated capacity values. The lower number is current capacity plus any queued upgrades; the lower-right value is the next upgrade cost, or MAX. These values remain visible, including the unusual native default first-group cost. No price/gameplay data was changed.

## Accept frame

The affected control is the in-run tabbed ship window's dynamically labeled Accept button. Its Shiny base artwork has a different interior contour from the native TextButton geometry. Only `upgradeUI/buttons_accept_base.png` falls back to the owned native frame, preserving the native label, hover fill and click region. Hangar customization's separate Accept images are retained.

## Enemy charge indicators

`CustomWeapons.cpp` already calculates the enemy corner using the combat-panel origin, target offset and ship corner. Its charge-bar safety clamp still used x=1273. Both guarded immediates now use `width - 7`, retaining weapon/mount placement, mirroring and the existing vertical bounds. No weapon, sensor or ship gameplay data is patched.

## Validation scope

At 3440×1440 Modded, isolated tests reproduced the original defects and checked right-side information/subsystem tips, information-toggle clicks, reactor normal/hover/queued/MAX states, ship Accept hover/click and a centered store. Sensor reveal was exercised, and the corrected indicators were observed on a two-weapon Rebel Rigger and a one-weapon Mantis Fighter after clearing the first target and changing beacon. Classic and Windowed restarts were checked. These are selected UI/encounter checks, not a full campaign.

The builder checks all 16 native fallback resources, 31 gameplay panoramas, six texture reconstructions and 651 unchanged base sprite resources. Installer/update tests cover corruption, stale inventories, copy-failure rollback, path validation, settings and current campaign/profile preservation.
