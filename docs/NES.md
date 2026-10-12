# Native NES emulator

The GRUB/i386 edition compiles InfoNES into `/NES.RUN`. Its 6502 CPU, PPU,
cartridge mapper and APU code execute inside BDS-OS, using the BDS framebuffer,
PS/2 keyboard, PIT and BDSFS. No host NES emulator, SDL, Linux process, or browser
is involved. The separate 8086 floppy edition cannot run this application.

Upstream is [jay-kumogata/InfoNES](https://github.com/jay-kumogata/InfoNES), pinned
to `fe3295c0a86bf5bbecc22aeab3b3af11f6f47908`. Its Apache-2.0 license and source
notices are retained in `third_party/infones/`. Mapper 90 uses the newer
pico-infonesPlus implementation at `eed95b61edd6d31ed0fc30410afad3e9a5e709ca`,
with its GPL-3.0 license and provenance alongside the InfoNES source. The
combined NES executable is supplied under GPL-3.0. The BDS integration is
`nes/platform.cpp`; `tools/build_nes.py` compiles it with freestanding i386 G++
and links without libc, libstdc++, SDL or OS imports. The resulting code/data
and BSS fit within the existing 8 MiB application slot. ROM data uses the heap.

## Launch and controls

Boot the updated ISO with the updated IDE disk attached. Press **F5**, then
**N** in Game Center to open the File Manager ROM picker. Select with arrows and Enter; Backspace changes to the parent directory and Tab selects another mounted volume.
File Manager opens `.NES` files with the emulator. The terminal supports:

```text
NES
NES /NES/cartridge.nes
RUN /NES.RUN /NES/cartridge.nes
```

| Action | Player 1 | Player 2 |
| --- | --- | --- |
| Direction | Arrow keys | I / K / J / L |
| A / B | X / Z | C / V |
| Start / Select | Enter / Tab | Space / B |

**P** pauses/resumes, **R** resets the cartridge, and **ESC** or **F10** exits
to BDS-OS. Nintendo World Championships uses player 2 Start (Space).
Both controllers track key press and release, including held buttons.

The NES image is 256x240, centered with integer scaling. Emulation targets
60 Hz; the kernel PIT has 10 ms precision and actual speed depends on the VM.
The emulated five-channel APU is mixed into mono PCM through the kernel’s AC’97/HDA driver. Unsupported audio hardware leaves it silent. Ctrl+F toggles fullscreen, and File → Open selects another cartridge after saving battery SRAM. No Zapper/light-gun support is implemented;
the Duck Hunt portion of the combination cartridge needs that peripheral.

## ROMs and saves

ROMs are user-supplied local files. Generic `seed.img` contains the emulator
without commercial cartridges. `data.img` is the user's persistent working
disk. Import only while the relevant QEMU/VirtualBox disk is **offline**:

```sh
make nes
python3 tools/nes_import.py build/data.img /path/to/first.nes /path/to/second.nes
```

Windows builds use `./build.ps1 nes`. WSL paths for Windows ROMs begin with
`/mnt/c/` or `/mnt/d/`. The importer adds the current `/NES.RUN`, copies
cartridges under `/NES/`, and writes `/NES/ROMS.TXT` for the picker. Cartridge
aliases use twelve SHA-256 hex digits plus `.NES` to fit BDSFS's 31-byte name
limit; `/NES/ROMS.TXT` retains readable titles and the File Manager lists the aliases. Re-importing merges the library.
BDSFS has 128 total entries, including
all applications, directories and save files. The importer checks sizes and
filesystem integrity before writing. Back up your image before importing.

Original iNES dumps with garbage in reserved bytes use the legacy mapper
decoding rule (ignore mapper high bits). Source ROM files remain unchanged.
Basic NES 2.0 cartridges are normalized in memory when their mapper number
fits eight bits, submapper is zero, ROM sizes use the ordinary byte fields,
RAM requirements fit the core, and peripherals/timing are supported. More
complex NES 2.0 hardware, VS/PlayChoice images, missing/empty/truncated ROMs and
unimplemented mapper IDs are rejected with a message. This is the InfoNES
mapper implementation; a listed mapper does not guarantee every game works.

Cartridges with the battery flag load/store raw 8 KiB SRAM in a neighboring
`.SAV` file. Changed SRAM is flushed every 300 rendered frames and on normal
exit/reset. A disk failure is reported; invalid existing save sizes are
rejected rather than overwritten. This covers in-game battery saves, not
emulator save states. Abruptly powering off can lose changes since the last
flush. QEMU `data.img` and a VirtualBox VDI are separate writable copies.

## Verification

`make test-nes` runs real QEMU acceptance tests with an original synthetic
6502 cartridge. It checks a rendered color change caused by a held controller
button, reset, battery SRAM across fresh boots, bad-ROM recovery, Game Center
and File Manager launches, and clean return to the desktop/filesystem.

Additional local compatibility checks use:

```sh
python3 tests/nes.py --rom-report build/nes-import.json
```

They load each supplied cartridge, render at least 120 frames, send Start and
controller input, check for a nonblank screen, capture the framebuffer and
exit normally. `build/logs/nes-results.json` records individual results.
These are boot/render/input smoke tests, not completion of every game or
proof of cycle-accurate behavior. The original test cartridge verifies saves
without relying on a commercial game's save menu.

On 2026-10-10 all 17 supplied cartridges passed these smoke checks. The
existing `make test`, `test-ui` and `test-doom` targets also passed with the
NES integration. The final `test-nes` run additionally checked compatible
NES 2.0 loading and rejection of an unsupported extended mapper header.
The local VirtualBox `bds-os` VM was verified with serial ports disabled:
filesystem mount, F5/N picker, Super Mario World rendering, and ESC return.
It was returned to its original powered-off state. Its new VDI retains all
11 existing regular files byte-for-byte; the previous VDI remains a backup.
Exact artifacts and checks are recorded in `build/logs/nes-verification.json`.

The vendored changes are marked in their files: the frame IRQ register array
index is relative to `$4000`, mapper 19's register array includes its third
index, and mapper 45's CHR bank array includes all eight banks. Mapper 90 was
replaced with the newer JY ASIC implementation; BDS omits its emulator
save-state hook assignments and declares the core's IRQ wiring symbol. The
remaining upstream core retains its original interfaces and mapper table.
