# ClassiCube for BDX-OS

This is a native port of ClassiCube 1.3.8, compiled from the pinned upstream
C sources, with its software renderer. It runs inside the BDS kernel without
Linux, Windows, SDL or OpenGL. It is an independent Minecraft Classic-style
game, not Minecraft Java Edition or Bedrock Edition.

ClassiCube requires an **SSE2-capable CPU** (Pentium 4/Athlon 64 or newer,
including the tested default QEMU and VirtualBox CPUs). Its rasterizer uses
SSE2 arithmetic; older CPUs receive a message instead of starting the game.

## Start and controls

Open Game Center with **F5**, then press **C**, or type `CUBE` in Terminal.
The app starts in a window. **Ctrl+F** toggles the BDS fullscreen presentation.
The default internal canvas is 400×300. **F9** cycles 320×240 performance,
400×300 balanced and 640×480 detail, saving the choice in `/CLASSIC/BDS.CFG`.
The canvas scales proportionally to the app window or screen. **F8** toggles
a 60 FPS limit and uncapped rendering for the current launch. New data images
default to a 60 FPS cap; existing options are retained.

- **WASD**: move; **Space**: jump; mouse: look around.
- **Left mouse**: remove a block; **right mouse**: place a block.
- **B**: block inventory; **1–9**: hotbar slot.
- **Escape**: pause/menu and release the mouse into the app window.
- **F10**: exit to BDS. Save your world first.

Use the game's **Save level** menu to name and save a world, and **Load level**
to reopen one. Files live under `/CLASSIC/maps`; options live in
`/CLASSIC/options.txt` on the volume containing CUBE.RUN. A new launch generates
a fresh world unless you load a saved one. The initial world is 64×64×64 and
the initial view distance is 32 blocks to fit the software renderer and heap.
Keep generated/imported worlds small: apps share a 64 MiB heap, and BDSFS has
128 total file/directory slots. An out-of-memory error terminates the game.

## Current limits

Generated-world benchmarks use a disposable VirtualBox 6.1.50 VM (one vCPU,
512 MiB, VBoxSVGA, 800×600 desktop and view distance 32), with throughput
checked against host wall time. Use 400×300 balanced quality for the 60 FPS
target; 640×480 detail can fall below it. Exact released-binary results are
in the dated release report. Scene generation, host load and machine speed
affect results; no universal 60 FPS guarantee applies. QEMU TCG software
emulation is much slower. See [modern platform changes](MODERN-PLATFORM.md)
and the benchmark scripts for the rendering improvements and test conditions.

This port supports **offline single-player**. Multiplayer, account login,
automatic downloads, plugins, clipboard integration, music and game sound
effects are not implemented in this port. Other BDS apps and system sounds
retain their existing audio support. The upstream settings/menu source is
included, but selecting an unavailable backend cannot enable it.

## Build and source

`make classicube` builds `build/apps/cube.run`; `make grub` also incorporates it
and the upstream ClassiCube texture pack into the generic seed image.
On Windows use `./build.ps1 classicube` with the documented WSL toolchain.
`make test-classicube` exercises the port in QEMU.

- Upstream: <https://github.com/ClassiCube/ClassiCube>
- Pinned commit: `33891a4e7e586941240261bc911312f228be57d3`
- Original sources/notices: `third_party/classicube/`
- BDS platform/window adapters: `classicube/`
- Build: `tools/build_classicube.py`

Upstream is BSD-3-Clause with the additional notices in its `license.txt`.
The BDS adapters and shared runtime retain the project's license. No Minecraft
commercial game files are downloaded or included. See `licenses/NOTICE.md`.
