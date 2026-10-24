# 8086 edition

Build with `make legacy` or `./build.ps1 legacy`. Boot `build/legacy.img` as a
1.44 MB floppy image in a BIOS VM. Its boot sector and kernel are assembled
with NASM `cpu 8086`; the kernel has a separate real-mode implementation.

Choose **1 Text Desktop**, or type `DESKTOP` at the command prompt. Numbered
shortcuts open Terminal, Files, Clock/System information, Games and Settings.
The desktop uses BIOS 80×25 text mode, so it works without the i386 framebuffer.

`SETTINGS` offers blue, green and light color schemes, a PC-speaker sound
toggle and test tone. `BEEP` plays the tone when enabled. Settings are for the
current session; this edition does not write its FAT12 floppy filesystem.

Existing DIR, TYPE and native 8086 RUN support remains. Use `HELP` for commands
and `RUN HELLO.RUN` for the included native program. The i386/x64 RUN formats,
ClassiCube, DOOM, browser, network/audio streaming and high-resolution graphical
desktop are not available on the 8086 kernel.

`python3 tests/legacy_desktop.py` tests navigation, colors, speaker controls,
FAT12 reading and returning from a native program in QEMU. Physical 8086
hardware and other BIOS implementations remain untested.
