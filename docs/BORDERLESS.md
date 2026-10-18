# Borderless fullscreen

Select **Borderless (Modded)** in Options, close FTL normally, and launch again.
The display cycle is Windowed -> Fullscreen (Classic) -> Fullscreen (Modded) ->
Borderless (Modded). Every display change requires a restart.

Borderless fills the primary desktop without a title bar or border, retains the
desktop resolution and refresh, and stays open when another app receives focus.
It uses the same ultrawide scenery, HUD and input layout as Fullscreen (Modded).
The existing display choices and default choice are retained. The guarded updater
preserves the saved preference.

For direct selection, run `FTL-Ultrawide.exe --borderless` in the installed game
folder. `--wide` selects Fullscreen (Modded); `--classic` is the recovery option.

## Scope

The same 1720x720 canvas and exact 43:18 aspect-ratio guard apply. Hardware testing
is at 3440x1440 only. Other aspect ratios, changing desktop resolution/monitors
while playing, HDR, mixed DPI and capture-tool compatibility are unvalidated.
Borderless does not add higher-resolution sprites or new gameplay behavior.

On the test host, Windows Graphics Capture sometimes returned a stale frame
until focus changed, while the game continued to render correctly on the monitor.
Capture and streaming tools need separate validation.

## Implementation

Saved mode 5 selects Borderless; rendering uses the existing immutable mode-4
path. Guarded setup changes remove the native one-pixel fullscreen fringe and
set the pinned SIL backend's refresh request to 0 and focus auto-minimize to off.
At the exact desktop size, SIL creates its popup window with no display-mode
change. No window-style helper, separate background service or new dependency is
installed. Other modes do not receive the Borderless setup changes.

The implementation is in [native/borderless.h](../native/borderless.h). The
[SIL reference source](https://github.com/PSP-Archive/System-Interface-Library/blob/master/src/sysdep/windows/graphics.c)
documents desktop-size mode selection, window creation and focus handling; the
specific owned 1.6.9 instructions are checked before patching.
