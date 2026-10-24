# Native games

The i386 edition includes ClassiCube, DOOM/Freedoom, InfoNES, five built-in
arcade/puzzle games and the new native 2048 port. The x64 preview currently
includes the five built-in games and 2048.

Open Game Center and press **P**, or run `RUN /PUZZLE.RUN`. Arrows move, U
undoes one move, N starts a new board, F2 saves, and Escape saves and exits.
The game restores `/PUZZLE.SAV` on the next launch. A read-only live volume
cannot save. Ctrl+F uses the standard app fullscreen control.

The move/merge rules are ported from Gabriele Cirulli's MIT-licensed
[2048](https://github.com/gabrielecirulli/2048). Pinned JavaScript sources and
license remain in `third_party/2048`; `apps/puzzle.c` contains the native C
implementation, BDX graphics, controls and persistence.

For ClassiCube, use F9 to cycle 320×240, 400×300 and 640×480 internal rendering,
and F8 to toggle a 60 FPS cap. See [controls, measured performance and
limitations](CLASSICUBE.md). FPS varies with the VM, CPU, world and view
distance. The port remains offline, with no game audio or GPU acceleration.

Windows games and DOSBox-X are still pending; see [compatibility status](COMPATIBILITY.md).
