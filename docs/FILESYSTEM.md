# Disk formats and integrity

## Volumes and resource locations

`DRIVES` lists discovered devices/volumes. `MOUNT n` changes the shell's current
drive; unqualified paths use that drive and folder. A path such as
`V1:/GAMES/TEST.WAD` addresses a particular volume for reads/writes without
changing the current drive. Numbers are assigned at boot and are not durable
device identifiers across hardware changes.

Native API file handles encode the originating volume; size/read/type restore
the caller's drive after operating on it. Treat handles as opaque and reacquire
them after structural directory changes. File pickers also restore their old
drive/folder on selection or cancellation. System app launches and default
icons use the boot/system volume. App-relative companion data resolves beside
the executable, independently of the drive browsed in File Manager.

The kernel supports qualified copy across volumes. Rename/move is restricted
to one volume; copy then verify before deleting when transferring between
drives. This interface is not a unified Unix mount tree. Detailed firmware and
filesystem limits are in [INSTALL.md](INSTALL.md).

## Legacy FAT12

The 1,474,560-byte image has 512-byte sectors, one sector per cluster, 65 reserved sectors, two nine-sector FATs, 224 root entries (14 sectors), 18 sectors per track, and two heads. Sector 0 is the custom boot/BPB; sectors 1–64 contain the kernel plus padding. FAT copies begin at sectors 65 and 74, the root at 83, and cluster 2 at 97. FAT12 is encoded conventionally with packed 12-bit entries; host tools can read it using the BPB rather than assuming one reserved sector.

The OS implements root listing, reads and FAT-chain traversal. It checks cluster bounds, file size, chain length, and BIOS read errors. Host creation is `python3 tools/floppy.py create build/legacy.img`. Seed/import definitions are in that script. Kernel writes, subdirectories, long names, fragmented writes and floppy formatting are not supported. A disk read is genuine storage access, but there is no claim of writable persistence from this edition's shell.

## BDSFS01 for GRUB

A deliberately small documented filesystem. It can occupy a raw image or a primary MBR partition and now uses the shared BIOS storage layer; see INSTALL.md. Default image: 64 MiB = 131,072 sectors. All integers are little-endian; all sectors are 512 bytes.

Sector 0 begins with the eight bytes `BDSFS01\0`, followed by total sectors (u32) and maximum entries = 128 (u32). The rest is reserved zero. Sectors 1–16 contain 128 entries of 64 bytes each. Data starts at sector 17.

| Entry offset | Size | Meaning |
| --- | --- | --- |
| 0 | 32 | NUL-terminated name, maximum 31 bytes |
| 32 | 4 | Parent ID; root = 0, other directory ID = entry index + 1 |
| 36 | 4 | Type: 0 unused, 1 regular file, 2 directory |
| 40 | 4 | File length in bytes |
| 44 | 4 | Starting data sector |
| 48 | 4 | Capacity in sectors |
| 52 | 12 | Reserved zero |

Names are case insensitive. Directories have no data extent. Every regular file occupies one contiguous extent. Writes allocate a new extent with at least eight sectors and write the data before replacing the metadata. Old extents become available for later writes. This preserves the old file during initial data writes, but the filesystem is **not journaled**, and metadata sectors are not an atomic transaction. Power loss or write failure during metadata updates may corrupt an image. Keep backups and use `CHECK` and host integrity tests; there is no repair tool yet. A disk-full error never prints false success. Maximum file size in the kernel is 32 MiB; there are at most 128 files/directories. Fragmentation can produce disk-full errors even with enough aggregate free space; no compaction is implemented.

The kernel validates table names, parent type and bounds, cycles, duplicate sibling names, file sizes/extents and overlapping extents at mount and on `CHECK`. It rejects malformed metadata. Operations include create/read/overwrite/copy/rename/move/delete, directory create/change/remove, and nonempty-directory deletion rejection. Data and metadata writes complete through BIOS services before success is reported. There is no separate firmware cache-flush command or power-loss durability guarantee. Persistence is tested across separate QEMU processes.

`tools/disk.py create` explicitly recreates a disk from current `.RUN` files and optional downloaded Freedoom data. It is a destructive operation against the named **image**. Normal builds regenerate `seed.img`, then create `data.img` only when absent, preserving saved work on subsequent builds. Tests use disposable copies. Import/export/list/check operations and examples are in README. Never edit a mounted image from the host. No operations target physical devices by default.
