# Verification record

## Desktop upgrade (October 12, 2026)

`make test-upgrade` passes 13 acceptance cases: four additional QEMU NIC
profiles with DHCP, ring-wrap HTTP transfers and cable recovery; mouse address
entry/Go, Ctrl+L and link navigation; external CSS/JS and DOM output verified
in framebuffer pixels; bounded runaway scripts; volume isolation and qualified
writes; embedded icon precedence; WAD selection on a second drive; and Start
restart/power off. `make test-resolutions` exercises ten additional menu sizes.
QEMU supplies nine requested sizes; 1366×768 falls back to 800×600 in the tested
firmware. The normal 800×600 path is covered by the other suites.

The native browser also resolves a hostname via DHCP-provided DNS and loads
an actual HTTP page from neverssl.com. The 16-case network/desktop suite and
eight audio captures pass with the updated kernel. The JavaScript engine has
host checks for string/number concatenation, fractional comparison and runaway
scripts, in addition to the guest DOM rendering check.

VirtualBox verification boots the new standalone VDI with no optical media,
checks HTTP/link/fullscreen navigation, browses another disk without changing
system resources, lists the preserved cartridge library, and shuts down using
Start. All 68 unchanged files, including 17 private ROMs and a battery save,
were compared byte-for-byte afterward; all new compiled apps match. The old
VDI is unchanged. Public release assets omit commercial ROMs, saves and private
VM disks. Exact installer checks and checksums accompany each release.

The NES File Manager assertion was updated to expect the new `V0:/` qualified
path. Physical PCs, unsupported PCI devices and modern web compatibility are
not covered by these results.

## Earlier checkpoints

The cloud desktop/Ethernet update adds `make test-network`: three-second
animated boot, wallpaper pixels, PNG/BMP viewer/fullscreen/damaged files,
WAD picker cancel/launch/loading, real DHCP/TCP HTTP rendering/link navigation,
redirect/chunked responses/error recovery and isolated AC97/HDA startup WAV
captures. Guest checks verify bulk reads at unaligned/sector/chunk boundaries
and EOF. Details: NETWORK-DESKTOP.md and the dated release report.


The desktop/audio update on 2026-10-11 adds separate tests for app windows and
Ctrl+F round trips, custom sidecar icons, companion BIN/DLL/DCL resources,
PSF fonts, BMP startup, and NES File Manager startup/File menu selection.
Actual QEMU WAV captures contain nonzero PCM for MP3, MIDI, DOOM soundtrack,
DOOM effects with music disabled, and an original NES APU tone cartridge.
AC’97 and HDA backends are exercised; details are in
`build/logs/audio-results.json` and `desktop-features-results.json`.

The existing VirtualBox VM also booted from the new standalone disk with no
DVD inserted. HDA STAC9221 initialized, MP3 displayed in its app window and
fullscreen, and the NES picker listed the supplied cartridges. The original
VDI remains unchanged; 24 existing files were verified identical in the new
VDI after the check. Physical hardware remains untested. See
[desktop/audio support and limits](DESKTOP-AUDIO.md) and the dated release
verification report for the final installer acceptance record.

Verified on this machine on 2026-10-10 using WSL Ubuntu build tools and QEMU PC emulation. The final regression transcript and SHA-256 manifest accompany the release. This file distinguishes tested behavior from implementation that has not been comprehensively exercised.

A subsequent VirtualBox 6.1.50 check found that absent COM1 hardware caused input polling to loop forever. The kernel now probes the UART scratch register and skips serial I/O when no UART exists. A new QEMU test reproduced the failure before the fix and passed afterward, checking desktop drawing, the mounted file listing, PS/2 keyboard input and a theme change with `-serial none`. The updated ISO also booted the actual VirtualBox VM with serial ports disabled and displayed `PASS: BDSFS mounted and validated` in its working desktop. The data VDI is IDE Primary Master, the mouse is PS/2, and the framebuffer controller is VBoxSVGA. Freedoom then loaded and rendered an actual level in the live VM; see `build/logs/virtualbox-doom-observed.png` and [VirtualBox setup](VIRTUALBOX.md).

## Verified behaviors

- Custom floppy boot, 8086 shell interaction through COM1, FAT12 DIR and TYPE, native Hello World, return to the shell, and unknown-command recovery. NASM enforces `cpu 8086` in every legacy assembly file.
- GRUB Multiboot2 boot, RGB framebuffer, PIT operation and BDSFS mount/integrity validation.
- Actual shell WRITE/COPY/MOVE/CD/PWD/TYPE/DEL/MKDIR/RMDIR operations and reading saved contents after a separate emulator boot. A nonempty-directory delete and a missing file report errors.
- Genuine native Hello World and calculator execution; invalid architecture and payload-checksum rejection without losing shell responsiveness.
- Framebuffer desktop and File Manager/Settings display, PS/2 keyboard switching, editor file creation and saved contents, and five native game loops with input, restart and exit.
- Separate game-logic checks for Snake collisions/growth, Pong paddle bounce/miss, blocks bounds/line clear, Mines layout/collision, and arcade shot hits.
- Host filesystem checks reject overlaps, cycles, oversize files, and disk exhaustion. Seed metadata and all native i386 headers/checksums validate.
- DOOM loaded from `.RUN`, initialized natively, read Freedoom from the IDE disk, entered a level, drew at least 100 frames, moved the engine player in response to PS/2 input, and returned cleanly so VER could execute. Framebuffer images show the actual level and return state.

`make test` covers boot/storage/loader/game invariants. `make test-ui` drives graphical input and captures screens, including the absent-UART regression; `make test-doom` drives the engine's level/input/exit acceptance. Tests use disposable image copies and do not change the working `data.img`. The original clean build and full rerun are recorded in `build/logs/regression.log`. The VirtualBox fix uses an incremental rebuild, with follow-up checks in `build/logs/virtualbox-fix-regression.log`; `build/logs/results.json` records the current artifacts and distinguishes the two verification runs.

## Limits of the tests

QEMU's default PC profile is the primary tested hardware environment. VirtualBox verification covers desktop boot, framebuffer drawing, IDE filesystem mounting and observed DOOM level rendering; controlled game/input/exit acceptance tests have not been run there. Original 8086, real PCs, UEFI, other disk controllers and unsupported framebuffer formats are not verified. The legacy edition has a read-only filesystem, one supplied native app, and a text game; it has no desktop or DOOM. The GRUB edition lacks process isolation and multitasking; the later desktop/audio and Ethernet update adds limited native audio and networking described above. Mouse and window interactions are basic, not a comprehensive accessibility or UI test. Tests demonstrate playable game logic and DOOM movement, not exhaustive gameplay, all maps, savegame round trips, or indefinite stability. Filesystem crash recovery is absent.
