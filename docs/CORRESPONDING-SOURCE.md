# Rebuilding release binaries

The repository and matching release source archive include the original BDS
kernel/apps, platform layers, build scripts and vendored sources for all app
engines, decoders and parsers. Follow [the README build steps](../README.md).
Pinned revisions and modifications are recorded in each upstream directory
and [licenses/NOTICE.md](../licenses/NOTICE.md).

The public ISO includes GRUB boot components generated using Ubuntu's
`grub-common` and `grub-pc-bin` packages. The October 12 build uses
**2.14-2ubuntu2**. Matching upstream source, Ubuntu patches and packaging are
distributed beside the ISO in a GRUB source archive. The BDS change to the
generated MBR is fully specified by `tools/installer.py`, which assembles
the boot-drive handoff stub and creates the disk image. Its generated
configuration comes from that script and `grub/grub.cfg`.

To retrieve the same distribution source with matching source repositories
enabled:

```sh
apt-get source grub2=2.14-2ubuntu2
```

The source package contains `debian/` build rules and the corresponding
upstream GRUB source. See its `INSTALL`, `README` and package build dependencies
for building GRUB itself. This project's build requires the resulting i386-pc
GRUB utilities/modules plus GCC, binutils, NASM, Python, xorriso, dosfstools
and mtools. Build/test package versions are recorded with each release.

Freedoom is separately licensed game data. `tools/freedoom.py` identifies the
exact downloadable release, checks its SHA-256 and retains its credits.
Public releases contain no commercial ROMs or IWADs.

Original BDS code is GPL-2.0-or-later. The combined browser is AGPL-3.0 and
the combined NES port is GPL-3.0; upstream Apache/BSD/zlib/CC0 notices remain
in place. Source is available here to users of the distributed binaries.
