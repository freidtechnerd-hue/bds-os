# Display and desktop preferences

Open **Settings (F6)**. Click **Display**, **Colors**, **Fonts**, or **Sounds**;
Tab cycles these categories. Other desktop windows remain accessible through
their taskbar buttons or F1–F7 shortcuts.

## Display

Choose a resolution using the mouse or arrow keys. **Apply / Enter** previews
it. **Keep / Enter** saves it; **Revert / Escape**, or waiting 15 seconds,
restores the previous mode. The list has Previous/Next pages when needed.
Settings offers only RGB32 modes reported by the video BIOS. Supported size
handling includes 720p, 1080p, 1680×1050, 1920×1200, 2560×1080, 2560×1440,
2560×1600, 3440×1440 and 3840×2160, as well as the smaller existing sizes.
This does not add a native GPU driver or guarantee these modes on every PC.

Apps keep their 800×600 logical drawing area. Ctrl+F fits that area to the
screen proportionally and centers it, with black borders where necessary.
Mouse input uses the same fitted rectangle; borders do not click app controls.
Game render detail is unchanged. Desktop windows and icons keep their pixel
dimensions. Wallpaper fills the desktop by cropping proportionally, while
the boot image fits inside the screen without distortion.

New installations automatically choose the monitor's preferred EDID resolution.
Supported aspect ratios include 4:3, 16:9, 16:10 and ultrawide. If the preferred
size is unavailable, Auto chooses the largest firmware mode that fits the
monitor and is within 2% of its aspect ratio; otherwise it keeps the boot size.
Invalid/missing EDID also keeps the boot size. This detects a monitor's preferred
pixel mode, not the size of the host VM window or its physical inches.

Choose **Auto detect / U**, then **Keep**, to save automatic selection. Existing
manually saved resolutions remain manual until you choose Auto. EDID base
blocks, CTA native detailed timings and DisplayID 1.3 preferred timings are
supported. QEMU standard VGA has a direct EDID fallback for firmware that
truncates extension blocks. Hotplug monitor changes require restarting BDS.

Normal boots restore `/DISPLAY.CFG` from the system volume. The GRUB
**Desktop 800x600 (ignore saved settings)** entry is the recovery route.
Higher resolutions use CPU rendering and require sufficient video memory;
start with 1280×720 or 1920×1080 if a larger mode feels slow. The desktop's
32 MiB canvas occupies RAM at 80–112 MiB; the installer payload starts at
112 MiB. Installed boots still require at least 128 MiB RAM, and the installer
requires 256 MiB.

## Colors, Fonts and Sounds

- **Colors:** cloud wallpaper or the amber desktop background. Color output
  stays RGB32 (8 bits for each RGB channel); this is not a 16/24-bit mode switch.
- **Fonts:** built-in, default PSF, bold PSF, or a PSF1 file selected in File
  Manager. The preview updates immediately. Glyphs are normalized to 8×8 cells.
  A selected custom font is copied to `/FONTS/USER.PSF` on the system volume
  so it remains available after removing its original media.
- **Sounds:** master volume, mute, system-chime enable/disable and a startup
  sound test. Master volume affects the PCM mix for all apps. The test honors
  mute, volume and the system-sound setting. AC'97/basic Intel HDA hardware is
  required; the tab reports when no supported output is detected.

These preferences save automatically to `/SETTINGS.CFG` on the system disk,
even while File Manager browses another drive. If saving fails, Settings
reports that the change lasts for this session only. Preferences loaded from
malformed files are ignored. App-menu font overrides are session-only.

VirtualBox custom sizes can be added while the VM is powered off, for example:

```sh
VBoxManage setextradata "bds-os" "CustomVideoMode4" "2560x1440x32"
VBoxManage setextradata "bds-os" "CustomVideoMode5" "3840x2160x32"
```

Use enough video RAM (64 MiB for a 4K RGB32 framebuffer). Firmware may still
decline a mode; only detected modes appear in Settings. There is no EDID
validation, hardware acceleration, automatic display resizing or DPI scaling.
