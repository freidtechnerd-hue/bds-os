# Install and use BDS-OS

This guide targets the **32-bit GRUB edition**. The separate `legacy.img`
floppy is an earlier 8086 text-only edition and does not contain the desktop.

## 1. Choose a setup

Start with **VirtualBox or QEMU** and a new virtual disk. These are the tested
environments. Real hardware needs legacy BIOS/CSM and compatible firmware;
physical PCs have not been tested. VMware/86Box and newer VM product versions
may work with equivalent emulated devices, but have not been verified.

| Setting | Recommended starting point | Limit or reason |
| --- | --- | --- |
| CPU | One x86 CPU | Native i386 apps; x87 is used by game/audio/browser code |
| RAM | 512 MiB | Installer minimum 256 MiB; installed OS tested with 128 MiB |
| Target disk | New 1 GiB virtual disk | Minimum 81 MiB; BDSFS expands up to 8 GiB |
| Firmware | Legacy BIOS | UEFI boot and Secure Boot are unsupported |
| Graphics | VGA/VBE, 800×600, 3D off | Other menu sizes depend on firmware support |
| Input | PS/2 keyboard and PS/2 mouse | USB Tablet and native USB input unsupported |
| Network | NAT, Intel PRO/1000 MT Desktop | DHCP; adapter PCI ID 8086:100e |
| Sound | Intel ICH AC'97 or basic Intel HDA | Analog PCM; no USB/HDMI audio |
| Storage | IDE initially | SATA/USB require BIOS INT 13h services |

No Guest Additions, Windows drivers, Linux drivers or host runtime belong
inside BDS-OS. The public build includes native applications and Freedoom;
bring your own NES ROMs or other compatible DOOM IWADs.

## 2. Download and verify

Open [GitHub Releases](https://github.com/freidtechnerd-hue/bds-os/releases).
Download the release asset whose name contains **Public** and ends in `.iso`,
plus its `.sha256` checksum. Source archives are for rebuilding; they are not
bootable installer media.

On Windows, compare this result with the value in the checksum file:

```powershell
Get-FileHash .\BDS-OS-Desktop-Upgrade-Public-2026-10-12.iso -Algorithm SHA256
```

On Linux, from the download directory:

```sh
sha256sum -c BDS-OS-Desktop-Upgrade-Public-2026-10-12.sha256
```

Use the matching filename if a later release has a different date. Keep an
unchanged copy of the ISO and back up the VM disk before experimenting.

## 3. VirtualBox installation

1. Create a VM named **BDS-OS**. Choose **Other / Other (32-bit)** or the
   equivalent unknown 32-bit OS profile. Skip unattended installation.
2. Assign **512 MiB RAM** and **one CPU**. Disable **EFI**. Leave ACPI enabled.
3. Create a new dynamically allocated **1 GiB VDI**. It must be a disposable
   dedicated disk, not an existing Windows/Linux data disk.
4. In **Storage**, attach that disk to an **IDE controller, Primary Master**.
   Attach the Public installer ISO to an optical drive. SATA optical boot
   has also worked in the tested VirtualBox VM.
5. In **Display**, select **VBoxSVGA**, disable 3D, and start at the default
   guest resolution. In **System**, use **PS/2 Mouse** as pointing device.
   Serial ports can remain disabled.
6. In **Network**, enable Adapter 1, choose **NAT**, open the advanced options,
   select **Intel PRO/1000 MT Desktop (82540EM)** and enable **Cable Connected**.
   The update also recognizes supported PCnet and other PRO/1000 models.
7. For sound, enable output with **ICH AC97** or **Intel HD Audio**. The actual
   tested VirtualBox host used HDA with its STAC9221 codec.
8. Start the VM. In GRUB choose **Install BDS-OS to a dedicated disk**.
9. Read the target list, select the **new disk**, check its size, and type the
   exact displayed **`ERASE <disk number>`** confirmation. This destroys that
   disk's previous partition layout and files. Esc cancels.
10. Wait for **INSTALL COMPLETE**. Remove the ISO from the virtual optical
    drive, restart, and boot from the virtual hard disk. You should reach the
    boot progress screen and desktop with no installer attached.

The installed disk contains GRUB, the kernel, applications and persistent
data. You do **not** need to attach `seed.img` or `data.img` when installing
from the Public installer. Those files are for the separate developer live
ISO workflow described in [VIRTUALBOX.md](VIRTUALBOX.md).

Names and locations of settings can vary by VirtualBox version. The project's
tested host version is 6.1.50; it is not a recommendation to downgrade your
host. Oracle describes the available emulated adapters and NAT in its
[VirtualBox manual](https://www.virtualbox.org/manual/ch06.html).

## 4. QEMU installation

Install QEMU for your host. On Debian/Ubuntu use `sudo apt install
qemu-system-x86 qemu-utils`. In a directory containing the Public installer,
create a new virtual disk (use a new filename):

```sh
qemu-img create -f qcow2 bds-system.qcow2 1G
qemu-system-i386 -m 512 -smp 1 -vga std \
  -drive file=bds-system.qcow2,format=qcow2,if=ide,index=0 \
  -cdrom BDS-OS-Desktop-Upgrade-Public-2026-10-12.iso -boot d \
  -netdev user,id=net0 -device e1000,netdev=net0
```

Follow steps 8–9 above. Once installation completes, close QEMU and cold-boot
the disk with **no `-cdrom` option**:

```sh
qemu-system-i386 -m 512 -smp 1 -vga std \
  -drive file=bds-system.qcow2,format=qcow2,if=ide,index=0 -boot c \
  -netdev user,id=net0 -device e1000,netdev=net0
```

These examples use shell line continuations; PowerShell users can put the
entire command on one line. QEMU's normal PC machine includes PS/2 input.
Its user networking needs no TAP setup or administrative network changes;
see [QEMU network documentation](https://www.qemu.org/docs/master/system/devices/net.html).

To try the additional adapters, replace only `e1000` with `rtl8139`, `pcnet`,
`e1000-82544gc` or `e1000-82545em`. Do not select `e1000e`, `virtio-net`,
`vmxnet3`, an RTL8169 or an arbitrary modern Intel adapter: those are different
devices and have no BDS driver.

Optional sound on a Linux host with PulseAudio-compatible output:

```sh
# Add these arguments to either QEMU command:
-audiodev pa,id=sound0 -device AC97,audiodev=sound0
```

Use an audio backend supported by your QEMU build/host (`qemu-system-i386
-audiodev help`). The acceptance suite also checks Intel HDA and captures WAV
output; see [QEMU invocation options](https://www.qemu.org/docs/master/system/invocation.html).

## 5. Other virtual machines

If trying VMware or another emulator, choose BIOS, one x86 CPU, PS/2 input,
VGA/VBE graphics, an IDE disk, and an emulated NIC from the table in
[NETWORK-DESKTOP.md](NETWORK-DESKTOP.md). Use the same dedicated-disk ISO
installation procedure. A device called “Intel Ethernet” is not sufficient:
its emulated model and PCI ID must match the driver.

UEFI-only VM configurations, ARM guest machines, paravirtualized storage,
virtio networking and USB Tablet input are unsupported. Hyper-V's usual
synthetic devices do not match this supported profile; use QEMU or VirtualBox
for the documented setup. These other VM products have not been tested.

## 6. Prepare a USB stick or CD/DVD

For a USB stick on Windows, use [Rufus](https://rufus.ie/) from its official
site. Select the correct removable stick and the Public ISO. When offered a
choice, write in **DD Image mode** so the hybrid image's boot layout remains
intact. This erases the stick. See the [Rufus FAQ](https://github.com/pbatard/rufus/wiki/FAQ)
for the distinction between ISO extraction and DD image writing. BDS needs
the latter raw-image layout. Copying the ISO onto a normal USB filesystem
does not by itself make that stick bootable.

On Linux, a graphical disk utility's **Restore Disk Image** action can write
the ISO to the whole selected stick. Double-check the destination device and
its capacity before accepting erasure. There is no need to manually create
partitions or format the stick first.

For optical media, use **Burn disc image** in a disc-burning program and
select the ISO. Do not create a data disc containing the ISO as a file. The
installer fits on an ordinary 700 MB CD or a DVD; an actual optical drive
must be available to the firmware. Physical USB/optical media were not tested
on real PCs; emulated optical and USB installation are covered by tests.

## 7. Install on a real PC

Use a spare machine and a **dedicated expendable HDD/SSD**. Back up anything
you need and disconnect unrelated disks before installation. This is a
whole-disk installer; it does not resize partitions or set up dual boot.

1. Confirm the machine can boot **legacy BIOS / CSM**. An exclusively UEFI
   machine will not boot this release. Select the legacy USB/CD entry in the
   firmware boot menu. Secure Boot is not a supported boot path.
2. Use PS/2 input, or firmware that reliably emulates PS/2 input. Enable legacy
   USB storage support if your firmware provides it. The OS has no native USB
   host-controller or hotplug driver; connect storage **before boot**.
3. Start with 512 MiB or more RAM, VBE graphics and a BIOS-visible IDE/SATA
   target. SATA access is through BIOS services, not an AHCI driver. NVMe
   availability depends entirely on legacy firmware and is not assured.
4. Boot the installer. Check the target disk's capacity/interface. A device
   absent from the disk list cannot be accessed by this OS.
5. Choose the dedicated target and enter its exact `ERASE` confirmation.
   Existing GPT layouts are refused. Do not use a disk holding your main OS.
6. Wait for **INSTALL COMPLETE**, remove the USB/CD/DVD and restart. Select
   the installed disk's legacy boot entry.

Hardware candidates must match implemented interfaces: BIOS INT 13h storage,
PS/2 input, VBE graphics, supported Ethernet PCI IDs, and Intel ICH AC'97 or
a basic analog HDA path. No particular physical PC, card or SSD model has
been acceptance-tested. There is no Wi-Fi driver. Sound codecs, firmware power
off and VBE modes vary, so start in a VM before choosing hardware.

## 8. First boot, networking and files

- Press **F1**, type `DRIVES`, then Enter to see detected disks/volumes.
  `MOUNT <volume>` changes the shell drive. File Manager and game pickers have
  independent drive selection; desktop icons/apps stay on the system disk.
- Type `NET` to inspect NIC, link, DHCP address, gateway and DNS. With NAT and
  a connected cable, DHCP should provide an address automatically.
- Press **F9**. Click the address bar or press **Ctrl+L**, enter a full HTTP
  URL (for example `http://neverssl.com/`), then press Enter or click **Go**.
  Bare hostnames are given an `http://` prefix. Some websites redirect to
  HTTPS and cannot load. Simple early-web-style pages work within the
  [documented HTML/CSS/JS limits](NETWORK-DESKTOP.md); age alone does not
  determine compatibility. Local HTM files also work.
- Press **F5**, then **D** for the DOOM WAD picker or **N** for the NES ROM
  picker. Use drive buttons or Left/Right, select a file, then Enter. DOOM
  initialization can take tens of seconds; its loading screen remains visible.
- BDSFS and FAT12/16/32 are writable. ISO9660 discs are read-only. NTFS, exFAT,
  ext4 and GPT volumes cannot be mounted. FAT uses 8.3 aliases. See
  [storage limits](INSTALL.md#storage-access) before copying valuable files.
- `.BIN`, `.DLL` and `.DCL` are companion **data**, not Windows libraries.
- Choose the desired resolution in the installed GRUB boot menu. If firmware
  cannot supply it, select 800×600. Ctrl+F changes app fullscreen, not the
  physical screen mode.
- Press **F10**, then **P** to power off or **R** to restart. Wait for writes
  to finish. On unsupported ACPI firmware, use the VM/physical power control
  after file activity has stopped.

To add ROMs, copy files from a boot-time attached FAT volume using File Manager
or shell `COPY`, or use [the offline import tools](NES.md). Never mount or edit
the same image simultaneously from the guest and host. Public builds contain
no commercial games; personal ROMs and saves should stay out of public forks.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| No boot device / EFI shell | Disable EFI, use the installer ISO as boot media, select legacy boot |
| Installer refuses RAM | Set at least 256 MiB; 512 MiB is the recommended starting point |
| Target missing | IDE controller, BIOS INT 13h exposure, storage connected before startup |
| Installer refuses GPT | Use a separate blank/MBR experimental disk; it will not convert GPT |
| Desktop has no apps | Public installer must finish; developer live ISO needs its data image |
| Mouse does not move | Choose PS/2 Mouse, not USB Tablet; click inside the VM window |
| Network adapter not found | Match a listed PCI model; disable unsupported alternative NICs |
| No DHCP or cable disconnected | NAT enabled, Cable Connected on, host connectivity working |
| Page stays local | Focus address with Ctrl+L, enter URL, then Enter/Go; read the bottom status |
| HTTPS error / page looks plain | Browser supports HTTP and a small CSS/JS subset; use a simpler page |
| No sound | AC'97/HDA selected, host output enabled, `AUDIO` detects hardware |
| Black screen at a new size | Reboot, choose 800×600; firmware may lack the requested RGB32 mode |
| Old files after rebuilding | `data.img`/VDI are preserved copies; refresh offline and rebuild installer |

The technical [installer guide](INSTALL.md) explains the disk layout,
integrity checks and firmware constraints. [Verification](VERIFIED.md) records
test scope; successful VM checks are not a claim of universal hardware support.
