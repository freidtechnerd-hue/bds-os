![BDX-OS logo](assets/branding/readme-logo.png)

> **October 23 update:** TrueType fonts, supplied logos/icons/sounds, graphic window controls, faster ClassiCube, native 2048, an x64 preview and an updated 8086 desktop. See [release notes and verification](docs/RELEASE-2026-10-23.md).

# BDX-OS — Binary Development eXperience

A bootable x86 hobby operating system created by **freidtechnerd-hue with
ChatGPT GPT-6.1 Sol and GPT-6 Astra**, as featured in
[the original YouTube video](https://www.youtube.com/watch?v=oUb6CiOTym4&t=10s).
Development continued after the video was released; this repository contains
the updated source and installation instructions. See [credits](docs/CREDITS.md)
and [changes since the video](docs/CHANGELOG.md).

The main edition boots through GRUB into its own 32-bit kernel and desktop.
It does not launch Windows or Linux. QEMU/VirtualBox emulate hardware;
Linux or WSL supplies build tools. Separate [x64 preview](docs/X64.md) and [8086 floppy](docs/LEGACY.md) editions are included.
The project was previously named **BDS-OS**; existing disk and RUN formats retain their identifiers.

**Start here:** [installation tutorial](docs/GETTING-STARTED.md) covers
VirtualBox, QEMU, other VM software, USB/CD/DVD preparation, real hardware,
networking and troubleshooting. Download the **Public installer ISO** and its
SHA-256 file from [Releases](https://github.com/freidtechnerd-hue/bdx-os/releases).
The installer erases one explicitly selected dedicated disk. That disk then
boots without the USB stick or optical media.

![BDX-OS desktop and TrueType settings](docs/screenshots/bdx-font-settings.png)

## Features

- Supplied BDX logos and transparent XP icon theme, navy title bars, graphic close/minimize/maximize controls,
  DejaVu antialiasing and [TrueType file loading](docs/FONTS.md), shared clipboard,
  editor shortcuts and a live system monitor.
- Desktop, cloud wallpaper (4K cache on x64), custom cursor, three-second boot screen with
  progress bar, Start menu, and startup/system sounds.
- Terminal, File Manager, editor, calculator, system information, five small
  games, BMP/PNG viewer and a [Media Player](docs/MEDIA.md) for music, MPEG video,
  unencrypted DVD titles with authored menu navigation and autoplay, and
  BIN/CUE audio CDs (SSE2 required).
- Native DOOM with Freedoom, WAD picker, loading screen, music and effects;
  native [2048](docs/GAMING.md), and the InfoNES emulator with ROM picker and battery saves. Apps start in
  windows; **Ctrl+F** toggles fullscreen.
- Native [ClassiCube](docs/CLASSICUBE.md) with software rendering, offline
  single-player, world saves and mouse/keyboard controls (SSE2 CPU required).
- HTTP/HTTPS browser compiled with Gumbo, lwIP, BearSSL and Elk: clickable address bar,
  links, basic CSS and limited JavaScript/DOM support. **Modern web-app and PC-layout compatibility remain incomplete.**
  HTTPS requires RDRAND and a valid UTC clock; see [requirements](docs/HTTPS.md).
- DHCP for selected Intel PRO/1000, Realtek RTL8139 and AMD PCnet adapters;
  AC'97 and basic analog HDA sound.
- BIOS-discovered disks and boot-time USB storage; BDSFS/FAT12/16/32 read/write,
  ISO9660 reading. Drive browsing preserves system app resources.
- Native `.RUN` files with embedded icons (16×16 i386; 32×32 x64), custom PSF1/TrueType fonts and
  companion BIN/DLL/DCL data. Windows DLL execution is not supported.
- Start-menu restart/power off and Settings resolution selection from 640×480
  to 3840×2160, including 16:9/16:10 widescreen, subject to firmware availability, with preview, automatic
  revert, saved preferences and a GRUB recovery option.

Public builds include Freedoom with its license and credits. **Commercial NES
ROMs, commercial DOOM WADs, personal saves and private VM disks are excluded.**
Supply your own compatible game files. See [NES](docs/NES.md),
[DOOM](docs/DOOM.md), [desktop controls](docs/DESKTOP-UPGRADE.md), and
[browser/network limits](docs/NETWORK-DESKTOP.md).

![Native browser rendering its CSS and JavaScript acceptance page](docs/screenshots/browser.png)

## Build from source

On x86-64 Debian/Ubuntu, install the tools and build a generic, ROM-free installer:

```sh
sudo apt update
sudo apt install build-essential gcc-multilib g++-multilib binutils nasm grub-pc-bin grub-common \
  xorriso qemu-system-x86 python3 dosfstools mtools
make freedoom
make all
python3 tools/installer.py --data build/seed.img
```

`freedoom` downloads pinned Freedoom 0.13.0 and verifies its SHA-256. Native
engine/parser/decoder source is vendored under `third_party/`, with provenance
and licenses. The compiler emits freestanding i386 code with a BDS runtime.
The NES port is InfoNES, not FCEUX.

On Windows, install these tools inside a WSL distribution named **Ubuntu**,
then run `./build.ps1 freedoom` and `./build.ps1 all` from PowerShell. Build a
generic installer using the Python command above inside WSL. The PowerShell
`installer` target packages the current **personal** `build/data.img` instead.

| Artifact | Purpose |
| --- | --- |
| `build/bds-os-installer.iso` | BIOS USB/CD/DVD installer generated from the selected data image |
| `build/bds-os.iso` | Developer live kernel ISO; attach a separate data image for apps |
| `build/seed.img` | Rebuilt generic 64 MiB BDSFS app/data image |
| `build/data.img` | Persistent working image; ordinary rebuilds preserve it |
| `build/apps/*.run` | Compiled native i386 applications |
| `build/legacy.img` | Separate 1.44 MB 8086 bootable floppy |
| `build/x64/bds-os-x64.iso` | Native 64-bit live preview (build with `make x64`) |
| `build/x64/seed.img` | Copy before use as a writable x64 primary IDE data image |
| `build/logs/` | Local test logs, captures and verification reports |

`make clean` deletes the entire build directory, including saves in `data.img`.
Back it up first. Update an existing **offline** working image with
`make desktop-data`; it refreshes managed apps/assets and retains other files.
Never edit a disk image from the host while a VM is using it.

## Using the desktop

F1–F9 open Terminal, File Manager, Editor, System Information, Game Center,
Settings, About, Image Viewer and Web Browser. **F10** opens Start; its **R/P**
items restart/power off. In DOOM, F10 exits the game instead. File Manager and
game pickers use Left/Right or drive buttons to switch devices. Enter opens
an entry and Backspace goes to its parent. **Ctrl+L** focuses the browser
address field; Enter or Go navigates. **Ctrl+F** toggles app fullscreen.

**F6 → Settings:** select a screen size, Apply, then Keep within 15 seconds.
Escape or the timeout reverts it. Kept sizes are restored on normal desktop
boots. Use GRUB’s **Desktop 800x600 (ignore saved settings)** to recover.

[Display, Colors, Fonts and Sounds settings](docs/SETTINGS.md).

![Settings resolution controls](docs/screenshots/widescreen-settings.png)

![Widescreen choices in VirtualBox](docs/screenshots/widescreen-settings.png)

**Media Player:** open its desktop icon or press F10, M. O opens a file,
D starts an unencrypted DVD with its authored menus, C checks for a legacy IDE audio CD, and
Space pauses. BIN/CUE images work through the file picker. See the
[format, controls and drive guide](docs/MEDIA.md). CSS-encrypted discs and
subtitle/SPU overlays are not supported.

![Downloaded public-domain recording playing from a BIN/CUE test disc](docs/screenshots/media-cd.png)

![Native ClassiCube loading a saved world](docs/screenshots/classicube.png)

Useful terminal commands:

```text
HELP
DRIVES
MOUNT 0
DIR
WRITE /NOTES.TXT "Hello from BDX-OS"
TYPE /NOTES.TXT
WEB http://neverssl.com/
NET
DOOM
NES
CHECK
```

Use paths such as `V1:/GAMES/GAME.WAD` to reference another volume without
changing the shell drive. Apps run one at a time and return to the desktop.
[Storage](docs/FILESYSTEM.md), [native RUN ABI](docs/RUN.md), and
[architecture](docs/ARCHITECTURE.md) explain the implementation.

## Tests and limitations

```sh
make test test-ui test-doom test-nes test-desktop test-storage
make test-audio test-network test-internet test-upgrade test-resolutions test-display-settings
make test-classicube
python3 tests/modern_desktop.py
python3 tests/bdx_theme.py
make test-x64
python3 tests/legacy_desktop.py
# Requires host FFmpeg for original generated test fixtures:
make test-media
# Test the currently generated installer without rebuilding its data:
python3 tests/install.py
python3 tests/install_safety.py
```

Tests boot actual binaries in disposable QEMU images. They check persistence,
loaders, input, gameplay, PCM sound, HTTP, multi-drive access, installation and
cold boot without installation media. See [verification](docs/VERIFIED.md).
VirtualBox has also been exercised; physical machines and other VM products
remain untested.

This is a single-process, cooperative system with trusted ring-0 native apps.
It has no user accounts or app isolation, no UEFI boot, no native USB/AHCI/NVMe
drivers, and no NTFS/exFAT/ext4 or GPT volume support. The i386 storage path depends on BIOS
INT 13h; x64 currently has only a native primary-IDE BDSFS path. USB hotplug and USB-only input are unsupported. The installer needs
at least 256 MiB RAM; the installed OS is tested at 128 MiB. Use a VM or a
dedicated experimental disk and keep backups.

DOSBox-X and DOS/Windows 3.1/95 binary execution are **not implemented** in this release.
See the [compatibility status](docs/COMPATIBILITY.md) before choosing an edition.

Original BDS/BDX code is **GPL-2.0-or-later**; see [COPYING](COPYING).
The combined NES executable is GPL-3.0 and the combined Elk browser is
AGPL-3.0. Upstream notices, asset attribution and corresponding source
information are in [licenses/NOTICE.md](licenses/NOTICE.md).

![Authored DVD menu in BDS Media Player](docs/screenshots/dvd-menu.png)

Big Buck Bunny DVD artwork: © 2008 Blender Foundation, CC BY 3.0.
[Disc source and attribution](docs/MEDIA-TEST-SOURCES.md).
