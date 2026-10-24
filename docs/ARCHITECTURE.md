# Architectures and hardware boundaries

## 8086 target

NASM flat binaries with `cpu 8086`. BIOS loads `legacy/boot.asm` at physical 0x7C00. The boot sector contains a FAT12 BPB and loads 64 reserved sectors one at a time using INT 13h CHS, to 0x1000:0000. It jumps there with DL preserving the boot drive. Kernel DS/ES/SS become 0x1000; the stack starts at offset 0xFFFE. It uses BIOS text, keyboard, floppy, RTC and conventional-memory APIs plus optional COM1 input/output for emulator tests. No 32-bit registers, protected mode, paging, or GRUB are used.

The shell buffers commands, maps its implemented operations, reads root directory/FAT entries from the actual disk, follows FAT12 chains, and loads a native file at 0x2000:0000. Applications are trusted real-mode code and return with RETF. Root files are limited to 60,000 bytes. The floppy geometry is specifically 80 cylinders, two heads, 18 sectors. BIOS services are assumed. Writes and nested directories are deliberately unavailable and report unsupported operations.

## GRUB target

NASM ELF32 entry, freestanding GCC `-m32` code, and GNU ld `elf_i386`. This is the 32-bit x86 ABI; the current GCC defaults and DOOM's x87 runtime are tested on QEMU's default x86 CPU, not on a literal 80386 machine. Multiboot2 supplies memory information and framebuffer metadata. The kernel checks the boot magic and a usable RAM interval from 1 MiB through 112 MiB, then installs its own flat GDT and IDT, remaps the 8259 PIC, and starts the PIT at approximately 1000 Hz. Timer IRQs are enabled; keyboard and PS/2 auxiliary bytes are polled. Faults log to COM1 and halt rather than continuing with corrupted state.

The kernel enables SSE state on CPUs advertising FXSR/SSE, initializes MXCSR,
and preserves x87/SSE state across firmware calls. C entry stacks and heap
payloads have 16-byte alignment. ClassiCube uses SSE2 arithmetic and checks
CPU/kernel support before entering its compiled game code. Exception logs
include the vector, error code, instruction address and x87 status.

Memory layout:

| Range | Use |
| --- | --- |
| 0x00100000 onward | Kernel ELF, metadata, 256 KiB stack |
| 0x00800000–0x00FFFFFF | One fixed native executable slot (8 MiB maximum) |
| 0x01000000–0x04FFFFFF | 64 MiB block heap; split/free/coalesce, reset between apps |
| 0x05000000–0x06FFFFFF | Persistent desktop canvas (up to 3840×2160), independent of app heap |
| 0x07000000 onward | Relocated installer payload (installer only) |
| GRUB framebuffer address | Hardware-visible RGB output, identity-addressed |

Paging is disabled. All pointers are physical/flat addresses. Apps receive a versioned function-pointer ABI and execute cooperatively with the kernel's privilege. The loader resets the heap and discards stale keyboard events on program boundaries. Returning from entry or invoking `api->exit` restores the loader stack; the heap is reclaimed. This is not memory protection against malicious/faulty applications. The current API version is 7; complete version-1/version-2 prefixes are preserved and new services are appended. Apps check the version before using new fields.

Video uses Multiboot2 RGB32 framebuffer tags, honors pitch and color-channel bit positions, and accepts resolutions from 640x480 through 3840x2160. The normal tested mode is 800x600x32. A reserved RAM canvas is copied to the framebuffer. Unsupported modes produce a text-console fallback. Settings enumerates validated VBE RGB32 linear-framebuffer modes and switches through the shared real-mode BIOS thunk. A matching framebuffer layout also identifies the restore mode when GRUB used a native video driver. A 15-second preview offers Keep/Revert; Keep writes DISPLAY.CFG on the fixed system volume. Normal desktop boots restore this preference; explicit GRUB resolution/recovery entries override it. INT 10h video and INT 13h storage calls explicitly select their interrupt in the shared thunk. No accelerated GPU driver is present. Graphical apps draw to an 800×600 virtual surface, presented in a desktop window or fullscreen. DOOM renders its own 320×200 buffer. The linker refuses kernels whose static data overlaps the application slot. Custom ICO/BMP/PSF1 loaders supply icons, boot graphics and fonts.

Storage uses BIOS INT 13h EDD through a protected/real-mode thunk. The kernel discovers firmware-exposed IDE, SATA, USB and optical devices at boot, then recognizes BDSFS, FAT12/16/32 and ISO9660 volumes. BDSFS/FAT are writable; ISO9660 is read-only. Native USB/AHCI/NVMe drivers, hot-plug and UEFI are not implemented. See INSTALL.md for tested configurations and limits.

Audio uses polling DMA output through Intel AC’97 or PCI HDA controllers. The kernel mixes queued PCM with minimp3 decoding or basic MIDI/MUS software synthesis. HDA discovers an analog codec DAC path with CORB/RIRB verbs. Drivers disable controller IRQ generation, avoiding conflicts with BIOS disk calls. Physical hardware remains untested; see DESKTOP-AUDIO.md for supported profiles.

Ethernet uses an original polled Intel PRO/1000, RTL8139 or AMD PCnet DMA driver and vendored
lwIP 2.2.1 in NO_SYS mode. Static receive/transmit rings and TCP/IP memory stay
below the 8 MiB app boundary. DHCP, ARP, IPv4, UDP, DNS, ICMP and TCP run in
the cooperative input/sleep/present path. The native text browser compiles
Gumbo HTML5 and bounded Elk JavaScript with a small CSS/DOM implementation; Image Viewer compiles LodePNG. See NETWORK-DESKTOP.md for formats,
controls and limitations.

## Source map

`legacy/` is the entirely separate real-mode build. `kernel/` contains the GRUB entry, hardware, drawing, filesystem, loader, shell and window system. `include/bds.h` is the application ABI. `apps/` contains native samples and five game implementations. `doom/` is the native platform/libc shim; `third_party/doomgeneric/` is the vendored engine. `tools/` creates images, packs executables and builds DOOM. `tests/` drives actual QEMU instances and tests filesystem/game invariants.

API 5 appends directory enumeration, relative PS/2 mouse movement/capture,
modifier state and RTC date/time. Directory enumeration restores the caller's
volume and working directory. ClassiCube uses these in its native platform and
window adapters, with a cooperative world generator and software rasterizer.
See [ClassiCube](CLASSICUBE.md) for port limits.

## Native x64 preview

`kernel64/entry.asm` enters long mode with identity-mapped low 4 GiB and 2 MiB
pages. The native 64-bit interrupt frame and System V AMD64 loader share the
C desktop/app API sources, compiled with 64-bit pointers. This is a separate
binary target; RUN architecture 3 is incompatible with architecture 2.

The live filesystem module is moved to 112–176 MiB. Its 4K wallpaper cache
occupies 176–208 MiB, separate from the fixed app slot, heap and canvas. The
preview requires 256 MiB. A raw primary-IDE BDSFS disk uses native ATA28 PIO;
no BIOS calls are made after long-mode entry. It has six native apps, but no
installer, networking, audio, optical/FAT browsing or i386 game engines yet.
See [x64 build/boot instructions](X64.md).

## Fonts and supplied sounds

FreeType rasterizes bounded memory-backed TrueType faces into persistent ASCII
atlases. The font survives app heap resets; library/face buffers are freed after
rasterization. The desktop still uses fixed character columns. Supplied system
WAV cues are cached outside the resettable app heap and mixed into PCM output
before the master volume/mute stage. They do not replace the music decoder.
