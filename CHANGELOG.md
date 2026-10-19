# Changelog

## 0.1.0-beta.6.1

- Correct the driver-profile test double to resolve Windows short TEMP aliases like the real driver/installer. The tested native launcher and presentation behavior are identical to beta.6.
- Publish the package after clean-runner verification; beta.6 has no release download.

## 0.1.0-beta.6

- Use the windowed backend and focus path for Borderless while retaining the ultrawide framebuffer/layout; keep rendering while unfocused.
- Add explicit `--nvidia-dxgi` to install/update: a guarded, reversible NVIDIA profile restricted to this installation. It resolved capture-related GPU memory growth and substantially improved app switching on the tested host.
- Verify fresh Start, first event, HUD/ship interaction, normal Save+Quit and Continue in every offered display mode. Preserve existing saves and display preference during updates.
- Keep the 60 FPS limit. The initial run-entry access violation remains unproven; the later NVIDIA capture failure is separately identified.

## 0.1.0-beta.5

- Add Borderless (Modded) to the display cycle and `--borderless` to the launcher. The saved choice applies on restart.
- Keep the existing ultrawide renderer at the exact desktop size, preserve desktop refresh, and disable automatic minimization on focus loss in Borderless mode.
- Preserve the current display preference during updates; defaults and other display modes remain unchanged.

## 0.1.0-beta.4

- Hide reactor column price labels in every display mode; capacity, next cost, queued upgrades and MAX remain visible. Upgrade and refund calculations are unchanged.
- Restore the native store Close frame to align its artwork with the current button hover/click area on Buy and Sell tabs. Other selected Shiny resources remain enabled.

## 0.1.0-beta.3

- Right-side tooltips use the active viewport edge, accounting for the outer translation of centered menus; hover and click coordinates remain native.
- Sensor-3 enemy weapon charge bars retain their ship/mount positions instead of clamping to the old 1280-wide edge.
- Native label-free upgrade/reactor textures replace incompatible older overlays. Legitimate Hyperspace column costs, capacity, next cost and MAX labels remain dynamically rendered.
- Restore the native ship-menu Accept frame used by the current text button.
- Add a hash-checked update command for existing production installations, with current-file/save backups, settings preservation and rollback. Fresh builds still require pristine owned input.

## 0.1.0-beta.2

- Corrected the interrupted-install test's comparison of Windows short and long path aliases on GitHub's clean runner. Product rendering and installation behavior are unchanged.
- Refreshed the source/download package to include that test correction; supersedes the initial beta.1 upload.

## 0.1.0-beta.1

First public Windows Steam beta for 3440×1440, Steam FTL 1.6.9 and Hyperspace 1.23.2.

- Classic/Modded fullscreen choices and restart-to-apply behavior.
- Expanded viewport; side-aligned HUD; menu, pause, modal darkening, information-button and loading-screen corrections.
- 31 complete gameplay panoramas, repaired title/hangar additions and a worker route following the walkway.
- Four upgraded dynamic textures, full-width environment/flash layers and corrected Credits scenery.
- Curated compatible Shiny UI; original ship/crew/weapon sprites retained.
- Removed in-game F9 art review, hints, comparison assets and artwork filename logging. Production developer console disabled.
- Original vanilla ships/statistics imported into `hs_ultrawide`, preserving originals.
- Portable local builder, backed-up installation and save-preserving rollback. No game files, saves or original upstream mod downloads distributed.
- Public Modded mode restricted to the approved 1720×720 canvas. Same-ratio scaling is expected, but only 3440×1440 has hardware testing.

Selected-menu/encounter testing is complete; full-campaign validation remains a beta goal.
