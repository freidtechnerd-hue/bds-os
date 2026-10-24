# October 23, 2026 — BDX-OS desktop and x64 preview

The project is now **BDX-OS (Binary Development eXperience)**. Existing BDSFS
and RUN identifiers remain compatible; historical releases retain their names.

- Supplied BDX logos, XP icon theme and startup/click/error/notification recordings.
- Native 32×32 x64 icons, transparent icon backgrounds, navy title bars and black boot surround.
- Graphic close/minimize/maximize buttons, window shortcuts, antialiased text,
  smoother wallpaper and a 4K wallpaper cache in the x64 edition.
- Native FreeType TTF loading, file picker and persistent font selection.
- Clipboard, editor selection/copy/cut/paste, live System Monitor and app API 7.
- ClassiCube direct-frame/SSE rendering improvements, F8 frame cap and F9 quality.
- Browser inline PNGs, back/forward history and address clipboard controls.
- Real x86-64 long-mode preview, native IDE BDSFS writes and six native apps.
- Native 2048 with undo and persistent saves in both i386 and x64.
- 8086 BIOS text desktop, themes and speaker settings.

DOSBox-X, Windows/DOS EXE execution, protected processes, preemptive multitasking,
GPU drivers and full modern web compatibility remain unfinished. The x64 build
is a preview with fewer drivers than i386. See [details](MODERN-PLATFORM.md).

# October 21, 2026 — Automatic display, DVD navigation and HTTPS

- Automatic preferred monitor resolution from validated EDID, including 4:3,
  widescreen and DisplayID 4K; saved Auto setting and manual override.
- Native libdvdnav menu VM, first-play/autoplay, arrow/Enter and mouse menu
  selection, title/program navigation and root-menu return on unencrypted DVDs.
- Certificate-validated native TLS 1.2, HTTP/HTTPS redirects and 2 MiB browser
  pages. Live Wikipedia fetch works; visual layout remains simplified.
- Full source and third-party license notices accompany this build.

# Development after the video

## October 19, 2026

- Proportional fullscreen and pointer mapping, cropped wallpaper, fitted boot image.
- Firmware mode handling through 4K, with paged Display settings.
- Display/Colors/Fonts/Sounds tabs, saved preferences and master audio volume.
- Reuse immutable optical directory metadata for cross-volume streaming reads;
  explicitly reopening a drive refreshes its directory.


The [YouTube video](https://www.youtube.com/watch?v=oUb6CiOTym4&t=10s) shows an
earlier stage. These notes cover subsequent development; they do not claim
that every listed feature or test was present in the video.

## Media and widescreen — 2026-10-18

- Native streaming MPEG-1/2 player with MP2, AC-3 and DVD PCM decoding.
- Unencrypted DVD title selection, WAV playback, BIN/CUE CD tracks, playlist,
  pause, seek, repeat, volume and aspect-preserving fullscreen controls.
- Experimental legacy IDE ATAPI audio-CD extraction; physical playback unverified.
- Desktop/Start Media Player entry, wider file filters and API 6 media services.
- 1360×768 mode option and widescreen labels in Settings; availability follows firmware.

## ClassiCube and display settings — 2026-10-17

- Native ClassiCube 1.3.8 with software rendering and offline single-player.
- Game Center shortcut, embedded block icon, world save/load and relative mouse input.
- Larger app/kernel stack and bounded temporary decoder buffers for this port.
- SSE2 ClassiCube arithmetic, aligned heap allocations and firmware FPU-state preservation.
- Recovery mode can save the current resolution to replace an invalid preference.

### Display changes developed October 14

- Live resolution controls in Settings, using supported VBE RGB32 modes.
- Keep/Revert preview with a 15-second timeout and persistent system-volume preference.
- Recovery boot entry and correct detection after GRUB native video initialization.
- Windows stay within the desktop after switching to a smaller resolution.

## Desktop upgrade — 2026-10-12

- Browser address input, Go button, Ctrl+L, HTTP host names, basic CSS and
  bounded Elk scripts with a small DOM bridge.
- Additional Intel PRO/1000 models, RTL8139 and AMD PCnet Ethernet.
- Volume-qualified files, independent game pickers, and fixed system resources.
- Embedded RUN icons, including packaging defaults for future apps.
- Start icon, restart, ACPI power off, and resolutions through 1920×1200.
- Public ROM-free distribution, source repository, and installation tutorial.

## Earlier post-video work

- Native InfoNES port, ROM picker and battery saves.
- BIOS USB/CD/DVD installer with standalone hard-disk boot.
- BIOS storage discovery, BDSFS/FAT writes and ISO9660 reading.
- Windowed apps, Ctrl+F fullscreen, icons, custom fonts and companion data.
- AC'97/HDA, DOOM effects/music, NES APU and MP3/MIDI playback.
- Cursor and boot artwork, three-second loading bar, cloud wallpaper,
  BMP/PNG viewer, DOOM picker/loading screen and system sounds.
- Initial Intel PRO/1000 DHCP and native HTTP browser.

The browser remains a small HTTP browser, not a Firefox port. Physical
compatibility is not established by VM tests. Release verification records
identify what was actually exercised.
