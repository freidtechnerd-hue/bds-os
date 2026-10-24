# Modern desktop and native x64 preview — October 23, 2026

This update improves the existing hobby OS. It does not provide Windows/Linux
binary compatibility, process isolation, preemptive multitasking or GPU drivers.
Native apps still cooperate in one privileged address space.

## Desktop and apps

- Antialiased DejaVu text in the existing eight-pixel columns, selected with
  Settings → Fonts → Smooth text. Original built-in and custom PSF fonts remain
  available. The setting persists on the system volume.
- Smooth, proportional wallpaper scaling. The desktop redraws when input or
  state changes, with a periodic clock refresh, instead of constantly repainting
  an idle display. The taskbar shows UTC from the hardware clock.
- F11 maximizes/restores the focused built-in desktop window; F12 minimizes it.
  Its taskbar button or function key restores it. Native RUN apps continue to
  use Ctrl+F for fullscreen; they do not run concurrently in the background.
- A shared, in-memory clipboard holds up to 4095 bytes across app launches.
  Editor Ctrl+A selects the document, Ctrl+C copies, Ctrl+X cuts, and Ctrl+V
  pastes. Oversized clipboard operations leave the document unchanged. The
  editor scrolls to keep its cursor visible and retains its 16 KiB limit.
- `RUN /SYSINFO.RUN` opens a live system monitor with architecture, API version,
  uptime, heap use, display dimensions and frame statistics.
- The app API is now version 7. Existing i386 API prefixes remain compatible;
  new calls provide clipboard, runtime metrics and synchronous direct frames.
  See [RUN API](RUN.md).

## Gaming

ClassiCube has three internal render sizes. F9 cycles **320×240 performance**,
**400×300 balanced** and **640×480 detail**, saving the choice in
`/CLASSIC/BDS.CFG`. Changing the desktop resolution scales the chosen canvas;
it does not silently increase the game's rendering cost. F8 toggles a 60 FPS
limit and uncapped rendering for this launch. New data images default to a
60 FPS limit; existing user options are preserved.

The renderer uses packed SSE interpolation, and the kernel offers a direct
frame path with one scaling pass. Captured gameplay reuses its window surround
and updates only its content rectangle. RGB framebuffers use bulk copies.
PIT timing now has approximately one-millisecond resolution. FPS reports count
actual submitted game frames after warm-up; a configured frame cap is not a
performance measurement.

Performance depends strongly on hardware, virtualization, world geometry,
view distance and selected quality. Results and exact binaries are recorded
by `tests/gaming_performance.py` (QEMU TCG) and `tests/vbox_gaming.py` (a new
disposable VirtualBox VM). No performance guarantee applies to arbitrary worlds
or physical machines. The ClassiCube port remains offline and has no game audio.

## Browser

- Actual inline PNG pixels for local and same-origin images, with transparency
  composited over the element background and proportional scaling.
- Bounds: four decoded images per page, 128 KiB encoded PNG, 1024×1024 input,
  displayed at most 480×224. The existing eight-resource budget is shared with
  scripts and stylesheets. Unsupported/failed images retain their alt text.
- `[` goes back, `]` forward, and R reloads. History stores up to 32 successful
  navigations in memory and is cleared when the app exits.
- Ctrl+C copies the address. Ctrl+V fills the address field; Enter/Go explicitly
  opens it. Pasted addresses are limited to 248 printable ASCII bytes.

This is still a bounded native browser, not Firefox. Cross-origin image hosts,
JPEG/WebP/SVG, full CSS layout, Unicode, browser-grade JavaScript/DOM, cookies,
forms and modern web applications remain incomplete. HTTPS still requires
RDRAND and a valid UTC hardware clock. See [HTTPS](HTTPS.md).

## Editions

The full i386 edition remains the main gaming/media/browser build. The new
[x64 preview](X64.md) shares its desktop and filesystem code but currently has
six native apps and fewer drivers. The [8086 edition](LEGACY.md) has its own
BIOS text desktop, settings and speaker support within real-mode constraints.

## BDX branding and fonts

The October 23 build incorporates the supplied BDX logos, an adapted XP icon
pack, TADA startup audio, click interactions, CHORD errors and DING notifications.
Sound samples mix with application audio and respect master mute/volume.
Desktop and native-app title bars use drawn close, minimize and maximize/fullscreen
symbols, independent of the selected font. A minimized native app pauses in a
restore view; this does not add concurrent background applications.
Active title bars are dark blue and the boot surround is black. x64 uses
native 32×32 icons, including RUN trailers; i386 keeps 16×16 icons. Alpha and
mask transparency preserve the wallpaper behind the icons.

[FreeType TrueType loading](FONTS.md) supports user-supplied TTF files and saves
custom font selection. The x64 wallpaper retains a 3840×2160 bitmap, generated
from the supplied cloud reference and resampled for the display asset. The i386
edition uses a 640×360 cache to preserve its 128 MiB memory profile.

[2048](GAMING.md) joins the native game collection on both i386 and x64.
[DOSBox-X and DOS/Windows compatibility](COMPATIBILITY.md) still need a host
runtime port and are not included as working features in this build.
