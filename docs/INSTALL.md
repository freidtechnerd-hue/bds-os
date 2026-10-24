# Install BDX-OS onto a dedicated disk

Download the public installer and SHA-256 checksum from the
[October 23 release](https://github.com/freidtechnerd-hue/bdx-os/releases/tag/bdx-desktop-x64-preview-2026-10-23).
The matching source archives and exact-ISO verification report are included.

For a guided walkthrough with recommended settings, see
[the VM and real-hardware tutorial](GETTING-STARTED.md). Public GitHub release
ISOs contain no commercial ROMs or personal saves. `make installer` uses your
local working data image; generic builds use `--data build/seed.img`.

`build/bds-os-installer.iso` is a **legacy BIOS hybrid ISO**. Write it to a USB
stick as a disk image (raw/DD mode), or burn it as a CD/DVD image. Copying the
`.iso` file onto an ordinary USB filesystem does not make the stick bootable.

Boot the media using the computer's **legacy BIOS / CSM** boot option, with
256 MiB or more RAM. Choose **Install BDX-OS to a dedicated disk**. Select a
disk from the displayed list, check its capacity and interface, and type the
exact `ERASE <disk number>` confirmation. ESC cancels. Installation destroys
the selected disk's existing partition layout and files; it is not a dual-boot
installer, partition resizer, or secure data erasure tool. Existing GPT disks
are refused; use a dedicated blank/MBR target.

The installer writes and reads back every block, then commits the MBR last.
It verifies the installation payload's CRC32 before offering any disk writes.
After **INSTALL COMPLETE**, remove the USB/CD/DVD and reboot from the installed
disk. The installed disk contains GRUB, the kernel, native apps, and persistent
data. It needs no other disk or installation media. The installed OS requires
128 MiB RAM in the tested profile. Minimum disk capacity is 81 MiB.

The local installer is personalized from `build/data.img`, including the NES
emulator and the supplied ROM library. Keep this personal image private.
To build a generic installer without the ROMs or personal working files:

```sh
make grub
python3 tools/installer.py --data build/seed.img
```

## Storage access

The kernel discovers firmware-exposed block devices and recognizable volumes
at boot. It reads raw BDSFS images and primary MBR partitions automatically.
`DRIVES` lists physical disks and volumes. `MOUNT <volume number>` switches the
current filesystem; ordinary DIR, CD, TYPE, WRITE, COPY, MOVE, DEL and MKDIR
commands then use that volume. In File Manager, Left/Right switches volumes.
The installed disk's MBR preserves its BIOS boot-drive ID for the kernel, so
its BDSFS is preferred even when another BDS disk is attached. Other boot paths
use GRUB's boot-device information, falling back to the first valid BDSFS when
the boot drive cannot be identified. Unqualified shell paths use the selected
volume. `V1:/PATH/FILE.TXT` addresses another volume without switching the
shell. File pickers restore their previous volume after selection/cancel;
system icons and built-in applications remain tied to the boot volume.

| Media / filesystem | Access |
| --- | --- |
| Firmware-exposed IDE / SATA HDD or SSD | BDSFS and FAT read/write |
| USB mass storage connected before BIOS boot | BDSFS and FAT read/write |
| FAT12, FAT16, FAT32 | Nested directories and files; 8.3 names |
| CD/DVD with ISO9660 | Read-only; ordinary ISO directory records |
| NTFS, exFAT, ext4, GPT volume discovery | Not implemented |

Drives use BIOS INT 13h EDD services through a protected-mode/real-mode thunk.
**There are no native AHCI, NVMe, USB host-controller or ATAPI drivers.** Hardware
support depends on firmware keeping those devices available through INT 13h.
Enable legacy USB storage support if applicable. This does not add USB keyboards,
hot-plug discovery, UEFI storage, optical burning, or arbitrary filesystem support.
UEFI-only machines are unsupported. A drive that is absent from DRIVES cannot
be installed to or accessed by this driver.

The driver accepts 512-byte disk sectors and 2048-byte optical sectors. It uses
32-bit LBAs and exposes at most 24 drives / 32 volumes. BDSFS has 128 entries,
31-character names, 32 MiB maximum individual writes and an 8 GiB capacity ceiling;
installation expands its data partition up to that ceiling. FAT directories have
a 128-entry snapshot and display existing long names by their short aliases.
ISO listings expose the first 128 entries; Joliet, Rock Ridge and multi-extent
files are unsupported. There is no journal or hot removal protection: wait for
operations to finish before powering off. These are hobby-OS drivers, not a
replacement for a general-purpose storage stack.

## Disk layout and rebuilding

- Sector 0: MBR and GRUB boot code.
- Sectors 1–2047: GRUB core with a tar memdisk containing the installed kernel
  and GRUB menu. This makes startup independent of external media.
- First partition: 16 MiB FAT16 containing reference kernel/config files.
- Second partition: BDSFS with native apps and data, expanded within its limit.

The embedded kernel is the one actually booted. Changing the reference
`/BOOT/KERNEL.ELF` file on FAT alone does not update the embedded kernel; rebuild
and reinstall for a kernel update. GRUB is built from the installed Ubuntu
distribution tools; its GPL license applies. Existing app/core notices remain
in `licenses/NOTICE.md`.

```powershell
.\build.ps1 installer
.\build.ps1 test-install
.\build.ps1 test-storage
```

The builder only creates regular image files. The acceptance tests create
disposable QEMU images, confirm cancellation does not change any bytes, install
from optical and USB media, detach that media, cold-boot the target, verify
persistent files, and exercise IDE, AHCI and USB firmware storage paths.
They never install onto a physical host disk.

Verified on 2026-10-11: optical and USB installation; detached-media disk boot;
IDE, AHCI/SATA and USB firmware read/write; correct boot volume with multiple
BDS disks; FAT12/16/32 guest operations with clean host fsck; read-only ISO9660;
all no-write safety cases; and boot/app/filesystem persistence regression checks.
Desktop, DOOM gameplay, NES rendering/input/SRAM, and drive detection/optical
browsing in the existing VirtualBox VM also passed on 2026-10-10 before the
final boot-drive identity fix. Physical machines have not been tested.
