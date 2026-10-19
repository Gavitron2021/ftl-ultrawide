# Borderless fullscreen

Select **Borderless (Modded)** in Options, close normally, then launch again.
The cycle is Windowed -> Fullscreen (Classic) -> Fullscreen (Modded) -> Borderless (Modded).
Display changes require a restart. The guarded updater preserves the saved choice.
`FTL-Ultrawide.exe --borderless` selects it directly; `--classic` provides recovery.

Borderless fills the primary desktop at its current resolution, without borders,
a title bar, topmost behavior or a display-mode request. Beta.6 uses the windowed
backend and windowed focus path while retaining the ultrawide framebuffer/layout.
Rendering continues when another app takes focus. Existing modes retain their paths.

## NVIDIA capture and app switching

On the tested Windows 11 / RTX 5080 / NVIDIA 616.92 host, the native OpenGL
presentation path produced long black app-switch transitions and capture stutter.
A captured crash failed in `nvoglv32.dll` with error 6 (GPU memory exhausted).
A repeat Discord-sharing test grew dedicated GPU memory to about 8 GB while CPU
memory stayed steady. Window/focus corrections alone did not resolve this.

The driver's **Vulkan/OpenGL present method: Prefer layered on DXGI Swapchain**
resolved that growth in the matched test. With the correction and this setting,
six minutes of Discord sharing stayed near 235–261 MB of dedicated GPU memory.
The tester reported smooth streaming, correct HDR appearance and a clear
Alt+Tab improvement. Rapid repeated switching combined with opening apps still
caused some stutter. This is one-host evidence, not a guarantee for every driver.

Enable it explicitly by appending **`--nvidia-dxgi`** to the normal `install` or
`update` command in [INSTALL.md](INSTALL.md). The installer uses the installed
NVIDIA driver API to create one profile restricted to the absolute game child
executable and this installation's marker file. It changes only this presentation
setting, never global settings. Existing user profiles are refused for manual
review. AMD/Intel users should omit the NVIDIA option; those capture paths have
not been hardware tested.

The timestamped installation backup records the profile transaction and a private
DRS backup. Its normal `restore --backup` command removes only the unchanged
profile created by that transaction; it never imports the whole driver database.
If you customize that profile afterward, restore refuses until you review it.
A repeated update keeps an already-owned profile; roll back the transaction that
originally created it to remove it. Driver rollback requires the NVIDIA API.
The driver setting applies at the next game launch; restart FTL after changes.

## Scope and implementation

The canvas is still 1720x720 with the same ratio guard. Hardware testing is only
3440x1440. Other ratios, mixed DPI, changing monitors/resolution during play and
other capture tools remain unvalidated. HDR appearance was user-checked on this
host with Windows Auto HDR enabled; no HDR pipeline or higher-detail sprites were added.
The game still renders OpenGL; the optional driver setting changes its presentation.
No frame-rate or gameplay-timing changes ship in beta.6.

[native/borderless.h](../native/borderless.h) checks the owned executable's hash
and instruction bytes before selecting a borderless popup with a minimize box.
[native/ui-layout.h](../native/ui-layout.h) retains mode 5 for Options comparisons,
uses mode 4 only for framebuffer/layout reads, and mode 0 for focus/fullscreen
handling. The [SIL reference](https://github.com/PSP-Archive/System-Interface-Library/blob/master/src/sysdep/windows/graphics.c)
and the owned instructions were inspected. The optional profile uses NVIDIA's
[public DRS API](https://docs.nvidia.com/nvapi/group__drsapi.html) and setting
`OGL_CPL_PREFER_DXPRESENT_ID` from [its public headers](https://github.com/NVIDIA/nvapi/blob/main/NvApiDriverSettings.h).
