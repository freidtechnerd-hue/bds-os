# VirtualBox setup

**New users:** follow [GETTING-STARTED.md](GETTING-STARTED.md#3-virtualbox-installation)
to install the Public ISO onto a new VDI and boot without the ISO afterward.
The live-ISO/raw-data conversion instructions below are a developer workflow.

## October 12 desktop upgrade

The existing development VM now uses `build/bds-desktop-upgrade-boot.vdi` as
BDS IDE Primary Master, with an empty optical drive. It booted independently
in VirtualBox 6.1.50, initialized HDA STAC9221 and Intel PRO/1000 DHCP, loaded
HTTP through Ctrl+L, followed a link, and returned to the local home page with
its JavaScript result visible. Browsing another physical disk's FAT volume
preserved system icons and built-in app launching. The NES picker retained
all 17 privately supplied cartridges. Start-menu Power off shut down the VM.

The prior `build/bds-cloud-network-boot.vdi` is retained unchanged as a backup;
the separate SATA disk is retained. Personal VM disks and cartridge data are
excluded from GitHub. Public users install their own new virtual disk.

## Earlier development checkpoints

The GRUB edition's desktop and BDSFS mount were verified in VirtualBox 6.1.50 on Windows on 2026-10-10. The kernel detects whether COM1 exists, so serial ports may remain disabled. Freedoom also loaded and rendered a level in the live VirtualBox VM. Controlled game/input/exit acceptance tests still use QEMU.

Create an Other/Unknown 32-bit VM with BIOS firmware, one CPU and at least 128 MiB RAM. Disable EFI and 3D acceleration. Use a PS/2 keyboard and PS/2 mouse; USB Tablet input is unsupported. The existing VM's VBoxSVGA controller successfully provided an 800x600 RGB32 framebuffer.

Mount `build/bds-os.iso` in the optical drive and put DVD first in the boot order. GRUB can boot the ISO through VirtualBox's SATA optical controller, and the kernel now uses firmware INT 13h for storage.

With the VM powered off, run this from the repository directory in PowerShell:

```powershell
& 'C:\Program Files\Oracle\VirtualBox\VBoxManage.exe' convertfromraw .\build\data.img .\build\bds-data.vdi --format VDI
```

Skip conversion if `bds-data.vdi` already exists and contains your saved files. In Settings > Storage, add an IDE controller and attach this VDI as **IDE Primary Master**, port 0, device 0. A newly created blank VirtualBox disk does not contain BDSFS or the applications. The data disk itself is not bootable: keep the ISO attached.

On boot, Terminal should show `PASS: BDSFS mounted and validated`. F1 opens Terminal, F2 opens File Manager, and F5 opens Game Center. If the disk warning remains, use DRIVES to inspect detected devices and volumes.

The VDI is a separate writable copy of `data.img`; saves in one image do not update the other. Back it up before rebuilding with `clean`, which removes the entire `build` directory.

## NES update

The earlier NES checkpoint used `build/bds-data-nes.vdi` on **BDS IDE, port 0,
device 0**, with the updated `build/bds-os.iso`. The new VDI was created from
the previous `bds-data.vdi`, preserving its existing files and adding the
native NES emulator and the supplied ROM library. The original VDI remains
available as a backup. Press **F5**, then **N** to choose a cartridge. See
[NES controls and save behavior](NES.md).

## Standalone installation

The 2026-10-11 desktop/audio update uses **`build/bds-desktop-cursor-boot.vdi`** on
**BDS IDE, port 0, device 0** in the existing `bds-os` VM. This replacement is
a standalone BIOS-bootable disk containing GRUB, the current kernel, apps and
the previous VDI's files; the optical drive can remain empty. Its disk-only
boot, HDA STAC9221 codec initialization, MP3 app window/fullscreen toggling and
NES File Manager picker were checked in VirtualBox 6.1.50. Actual PCM capture
tests use QEMU. The VM was returned to powered-off state after verification.

The original `build/bds-data-nes.vdi` and `build/bds-data.vdi` remain unchanged
as backups. All 24 existing data files, including 17 cartridges and battery
SRAM, were compared byte-for-byte after the VM check. The separate SATA disk
was not replaced. The VM configuration backup is
`releases/checkpoints/bds-os-before-desktop-audio-20261011.vbox`.

See [desktop/audio controls and formats](DESKTOP-AUDIO.md). This VDI's 64 MiB
BDSFS partition retains the old data capacity; installation to a larger disk
expands BDSFS within the installer limits.

Use at least 256 MiB RAM, BIOS firmware, and a disposable blank virtual disk. Boot `build/bds-os-installer.iso`, select the target and confirm erasure. When installation completes, detach the ISO and boot the disk. See INSTALL.md.
