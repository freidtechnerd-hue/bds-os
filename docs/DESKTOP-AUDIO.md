# Desktop, app assets and audio update

Graphical `.RUN` apps, including DOOM, NES, the editor, games and music player,
start in a desktop window. **Ctrl+F** toggles fullscreen and back without
restarting the app. The window has its app icon, a close button, and **File,
Options and Help** menus. Applications still run one at a time cooperatively.
The text utilities Hello, Calc and Sysinfo print to Terminal and return.

The pointer uses the supplied black-outlined white arrow, with a transparent
background and the hotspot at its upper-left tip. It is drawn above desktop
windows, file dialogs, and windowed/fullscreen app content.

**File → Open** opens a File Manager selection window. NES starts with this
window when no cartridge path is supplied; File → Open changes cartridges
after saving the current battery SRAM. Use arrows/Enter, Backspace for the
parent directory, Tab to select another mounted volume, or mouse selection
and double-click. ESC cancels. The player filters MP3/MID/MIDI; NES filters NES.
The editor refuses to replace a buffer containing unsaved edits.

**Options** toggles fullscreen, loads a custom font, restores the built-in
font, mutes/unmutes audio, starts MP3/MIDI playback, or stops music. Help writes
the controls to Terminal. ESC closes most apps; DOOM uses ESC for its own menu
and F10 to exit. The app window's close button follows each app's normal exit
path so NES can flush battery saves.

## Icons, fonts and companion files

All supplied apps and desktop tools have original **16×16 `.ICO` files** in
`assets/icons`. The loader accepts Windows ICO containing a 16×16, 32-bit
uncompressed DIB plus its AND transparency mask. PNG-compressed ICO is not
supported. Icons may be enlarged on the desktop while the stored file remains
16×16.

New RUN version-2 binaries embed their icon after the code/data payload.
For future apps, put MYAPP.ICO in assets/icons before building, or pass its
path as the optional fourth argument to runpack.py. The generic RUN icon is
embedded automatically otherwise. Embedded icons take priority; older
version-1 binaries may still use a sidecar. BIN, DLL, DCL, NES, music, font
and directory entries use associated icons on the fixed system volume.
See RUN.md for the binary layout and integrity checks.

Fonts are fixed-width **PSF1** bitmap files with glyph heights 8–32. The first
256 glyphs are normalized to the desktop's 8×8 cells; Unicode mapping tables
and alternate glyphs are not used. Supplied DEFAULT.PSF and BOLD.PSF derive
from the public-domain font already used by the kernel. Load through Options,
open a PSF file in File Manager, or use `FONT /FONTS/BOLD.PSF`; `FONT DEFAULT`
restores the built-in font. A font applies to kernel-rendered UI/text; DOOM
and NES retain the fonts rendered by their engine or cartridge.

**BIN, DLL and DCL are companion data files**, as requested. Their extensions
do not change their contents or execute code. A native app can call the new
`api->resource("EXTRA.BIN", buffer, capacity)` service to load data relative to
its own `.RUN` directory, or pass an absolute path. It returns the byte count
or -1 for missing/oversized/read-error cases. Apps interpret their own data
format; there is no automatic DCL parser or Windows DLL linker. Examples are
in `/DATA/EXAMPLE.BIN`, `/DATA/EXAMPLE.DLL` and `/DATA/EXAMPLE.DCL`.

## Audio

The kernel supplies 48 kHz stereo signed 16-bit PCM output through either:

- Intel ICH AC’97 bus-master controllers: device IDs 2415, 2425, 2445, 2485,
  24C5, 24D5, 266E and 27DE, including VirtualBox's AC’97 and QEMU's AC97.
- Standard PCI High Definition Audio controllers with 32-bit-addressable
  MMIO and 256-entry command/response rings. The HDA driver discovers a codec
  analog speaker, line-out or headphone path and configures a DAC and DMA
  output stream. QEMU `intel-hda` + `hda-output` is the reference test device.

These are native hardware drivers suitable for their supported register
interfaces, including physical hardware. Physical PCs have **not** been
boot-tested. HDA codec-specific GPIO/external amplifier quirks, jack switching,
HDMI/DisplayPort, USB audio, Sound Blaster and recording are not implemented.
An unsupported controller leaves games in silent mode. This update does not
change the BIOS/CSM boot and firmware-storage requirements in INSTALL.md.

Use `AUDIO` for availability, `PLAY /MUSIC/DEMO.MP3` or
`PLAY /MUSIC/DEMO.MID`, or open a music file in File Manager. The original demo
melodies are supplied with the image. PLAYER.RUN also opens a music picker
when launched without a path.

MP3 uses the pinned CC0 minimp3 decoder. MIDI supports SMF types 0/1, up to 64
tracks, 24 simultaneous software voices, tempo, note on/off, program selection
and channel volume. Instruments are simple waveform profiles, not a General
MIDI sample bank. Pitch bend, sustain and realistic drum instruments are not
implemented. DOOM MUS scores use the same synthesizer. Music files are loaded
into RAM and limited to 16 MiB; SMPTE-timed MIDI is unsupported.

NES mixes its emulated five-channel APU into mono PCM. DOOM mixes up to 16
DMX sound-effect channels and plays its MIDI/MUS music through the kernel.
The five small BDS games remain silent, but Options → Play MP3/MIDI can supply
background music. The shared audio API allows future native games to submit
PCM or play music. Playback stops and buffers are reclaimed when an app exits.

## Boot screen and rebuilding

The generated illustration is stored as `assets/BOOT.BMP`, an 800×600 RGB BMP,
and copied to `/BOOT.BMP` on the app disk. Replace that file with an
uncompressed 24/32-bit BMP (maximum 1024×768 and 4 MiB) to customize startup.
The runtime adds its status bar; malformed/missing images use a text fallback.
See `assets/README.md` for the image-generation prompt and asset provenance.

```powershell
.\build.ps1 desktop-data   # updates offline data.img, preserving other files
.\build.ps1 test-desktop
.\build.ps1 test-audio
.\build.ps1 installer
.\build.ps1 test-install
```

`desktop-data` backs up the image to `.before-desktop.img` on its first run.
A VirtualBox VDI is a separate working copy and must be updated from that VDI
to preserve saves made in the VM. Never modify a disk while its VM is running.
The installer still replaces its selected target after confirmation; it is
not an in-place updater for an installed physical disk.

## Implementation references

`kernel/appui.c`, `kernel/assets.c` and `kernel/video.c` implement windows,
icons, BMP and fonts. `kernel/audio.c` and `kernel/hda.c` implement DMA output;
`kernel/music.c` and `kernel/mp3.c` implement file playback. Native integration
is in `nes/platform.cpp` and `doom/sound.c`; the native API is in
`include/bds.h`. Tests capture actual PCM with QEMU's WAV audio backend.

The HDA register interface follows the
[Intel High Definition Audio specification](https://www.intel.com/content/dam/www/public/us/en/documents/product-specifications/high-definition-audio-specification.pdf).
Decoder provenance and licenses are recorded in `licenses/NOTICE.md`.

## Cloud desktop and browser update

See [NETWORK-DESKTOP.md](NETWORK-DESKTOP.md) for the three-second boot bar,
startup/system chimes, BMP/PNG viewer, DOOM WAD picker/loading screen and
Ethernet/HTTP browser. The native API is now version 4, retaining earlier
fields and adding app pointer input and independent volume access. See
[DESKTOP-UPGRADE.md](DESKTOP-UPGRADE.md) for the October 12 controls.
