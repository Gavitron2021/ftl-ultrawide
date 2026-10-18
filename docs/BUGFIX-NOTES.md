# Beta.4 and beta.3 UI corrections

The supported canvas remains 1720×720, scaled 2× at 3440×1440. Its added width is 440 logical pixels, with 220 logical pixels of centered menu padding per side. These are different transforms.

## Tooltips

Hyperspace 1.23.2 replaces `MouseControl::RenderTooltip` and retains the old 1260/1240 horizontal edges. The moved controls already use correct mouse/hit regions; the renderer was pushing their tips back toward the old edge.

The launcher supplies private bounds of `width - translation - 20` and `-(width - translation - 40)` to the two guarded horizontal-bound operands. The current outer translation is zero in ordinary combat and 220 for centered menus/focus windows. Native tooltip measurement, vertical clipping/scrolling, mouse coordinates and control hitboxes remain in use. Classic and Windowed use the original DLL bounds.

## Upgrade and reactor text

Older Shiny system/subsystem textures bake MAX lettering into the cost strip; reactor textures also bake POWER BARS/MAX. Current FTL/Hyperspace renders these labels dynamically. The final local overlay restores the owned native textures for those families. The rest of the selected UI remains enabled.

Beta.4 hides the optional Hyperspace prices above each reactor column. A guarded patch omits only the font-5 draw in `ReactorButton::OnRender` and its paired stack adjustment. The lower capacity value (including queued upgrades), lower-right next cost, and MAX remain dynamically rendered. This applies in Modded, Classic and Windowed modes. Upgrade prices, purchases, refunds and gameplay data are unchanged.

## Ship Accept and store Close frames

The affected control is the in-run tabbed ship window's dynamically labeled Accept button. Its Shiny base artwork has a different interior contour from the native TextButton geometry. The owned native `upgradeUI/buttons_accept_base.png` preserves the native label, hover fill and click region. Beta.4 extends the same correction to the store's separate `storeUI/store_close_base.png`, shared by the Buy and Sell tabs. Hangar customization's separate Accept images are retained.

## Enemy charge indicators

`CustomWeapons.cpp` already calculates the enemy corner using the combat-panel origin, target offset and ship corner. Its charge-bar safety clamp still used x=1273. Both guarded immediates now use `width - 7`, retaining weapon/mount placement, mirroring and the existing vertical bounds. No weapon, sensor or ship gameplay data is patched.

## Validation scope

Beta.4 additionally checks hidden reactor column labels, the 8-to-11 capacity price boundary (65 scrap), complete Undo refund, final 24-to-25 purchase and MAX, plus store Buy pages and Sell-tab Close hover/click. Modded 3440x1440 and Windowed 1280x720 were checked; outside/inside Close-edge clicks were checked in Windowed mode. The Classic layout shares the 1280-wide renderer but was not replayed for beta.4.

At 3440×1440 Modded, isolated tests reproduced the original defects and checked right-side information/subsystem tips, information-toggle clicks, reactor normal/hover/queued/MAX states, ship Accept hover/click and a centered store. Sensor reveal was exercised, and the corrected indicators were observed on a two-weapon Rebel Rigger and a one-weapon Mantis Fighter after clearing the first target and changing beacon. Classic and Windowed restarts were checked. These are selected UI/encounter checks, not a full campaign.

The builder checks all 17 native fallback resources, 31 gameplay panoramas, six texture reconstructions and 651 unchanged base sprite resources. Installer/update tests cover corruption, stale inventories, copy-failure rollback, path validation, settings and current campaign/profile preservation.
