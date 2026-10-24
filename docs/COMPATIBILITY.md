# DOS and Windows compatibility status

**This build does not execute DOS, Windows 3.1 or Windows 95 EXE binaries and
does not include a working DOSBox-X port.** Native BDX apps use the RUN format.
An x64 kernel by itself does not implement DOS interrupts, Windows APIs or
the binary loaders those programs need. `.BIN`, `.DLL` and `.DCL` remain
companion data files, as requested; Windows DLL imports are not implemented.

DOSBox-X is an appropriate separate emulation route: its upstream supports
DOS and DOS-based Windows guests. That would run a guest Windows installation
inside an emulated PC, rather than directly making BDX a Win32 OS. See
[upstream build requirements](https://github.com/joncampbell123/dosbox-x/blob/master/BUILD.md)
and [Windows 95 installation](https://dosbox-x.com/wiki/Guide%3AInstalling-Windows-95).

BDX still needs an SDL video/input/audio backend, the required C/C++ runtime
and file interfaces, and a memory/loader arrangement large enough for the
emulator before that port can be compiled and exercised as a native app.
The current fixed RUN slot is 8 MiB and its app heap is 64 MiB; the x64 preview
also lacks an audio driver. No host-built DOSBox executable is presented as
a native BDX build, and no untested EXE launcher is advertised as compatibility.

Acceptance for a future port must include real DOS COM/MZ applications,
keyboard/mouse and audio, persistent guest disks, a Windows 3.1 installation,
then a Windows 95 installation and named game tests. No universal Windows
software compatibility claim can follow from a single successful launch.
