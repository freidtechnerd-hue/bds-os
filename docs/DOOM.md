# Native DOOM port

Upstream: https://github.com/ozkl/doomgeneric

Pinned commit: `dcb7a8dbc7a16ce3dda29382ac9aae9d77d21284`.

License: GPL version 2 or later as stated in engine source headers; complete upstream GPL text and original copyright notices are retained. All sources required to rebuild the native executable are in this repository. `doom/platform.c` and `doom/libc.c` are BDS-specific. `I_Quit` has one BDS change adding native exit after the upstream shutdown callbacks.

`tools/build_doom.py` compiles the selected software engine files with freestanding 32-bit GCC, custom libc headers and no host libraries. It links them with `apps/link.ld`, converts ELF to raw payload, and wraps the result as `.RUN`. There are no Linux syscalls, SDL/X11 libraries, host processes, or host framebuffer/file services in the executable. The kernel supplies all runtime operations through `BdsApi`.

| Engine need | BDS implementation |
| --- | --- |
| Video | `DG_DrawFrame` blits the 320x200 software RGB buffer through kernel framebuffer |
| Keys | `DG_GetKey` maps PS/2 press/release events into DOOM keys |
| Clock | PIT millisecond ticks and kernel sleep |
| Memory | Kernel block allocator with libc malloc/free/realloc bookkeeping |
| Files | FILE objects backed by BDSFS; read buffers are cached in process heap |
| Exit/errors | Native loader exit restores kernel stack, frees process heap |
| Audio | Native DMX sound-effect mixing; kernel MIDI/MUS synthesis through AC’97/HDA |

The libc shim is scoped to the engine's needs, not a complete POSIX or ISO C implementation. Its formatted output supports the resource-name widths/precisions required by the engine, integer/string/float formats, and bounded snprintf. Scanning covers configuration integer/string fields. Optional external-dialog `system()` calls fail with an unsupported error; they cannot launch host software. Writable FILE buffers flush to BDSFS on close. Savegame/config operations exist but have not been fully regression-tested. F10 is an immediate BDS exit and bypasses engine-level config-save callbacks; the engine Quit menu invokes them.

Data: [Freedoom](https://freedoom.github.io/download.html) 0.13.0 Phase 1, BSD licensed. The pinned release archive SHA-256 is `3f9b264f3e3ce503b4fb7f6bdcb1f419d93c7b546f4df3e874dd878db9688f59`. Download tooling preserves COPYING and credits; the seeded disk includes them under `/DOCS`. Commercial DOOM game data is not bundled. Users can import their legally obtained compatible IWAD and pass its path with `DOOM /IWAD.WAD` or the RUN command. Only Freedoom Phase 1 episode 1 map 1 is acceptance-tested here; other IWADs/maps are not claimed verified.

QEMU acceptance uses a fresh copied image, invokes the actual `.RUN` loader with `-warp 1 1`, observes `GS_LEVEL` with a player object, captures rendered frames, injects a PS/2 Up press, observes changed engine player coordinates, captures the changed frame, exits with F10, and executes VER afterwards. This tests native load, initialization, WAD lookup, playable level, input, successive frames, clean return and fresh boot. Logs and genuine framebuffer captures are retained. It is stronger than a compile-only check but does not certify every game feature or long play session.

The audio module is original `doom/sound.c`, replacing the SDL sound backend at build time. Graphical output starts windowed; Ctrl+F toggles fullscreen. WAV capture tests cover soundtrack playback and a separate `-nomusic` run with firing to exercise sound effects alone. See DESKTOP-AUDIO.md for synthesizer and hardware limits.

`DOOM` and Game Center/D now open the WAD picker, with Freedoom selected by
default. Choose a base IWAD or cancel with ESC. A native loading screen appears
before engine initialization and shows WAD read progress. Direct IWAD paths
and RUN flags still work. See [cloud desktop update](NETWORK-DESKTOP.md).
