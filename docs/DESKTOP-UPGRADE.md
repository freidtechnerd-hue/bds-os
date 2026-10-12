# Desktop and browser update — 2026-10-12

Click the browser's white address box, type an HTTP URL, and click Go or press
Enter. Ctrl+L and F2 focus the box. A bare hostname gains http://. Basic CSS
and the Elk JavaScript subset are described in NETWORK-DESKTOP.md.

File Manager and app Open dialogs show the drive and previous/next buttons.
Use Left/Right, or Tab in an Open dialog. Unmountable volumes are skipped.
Switching devices no longer redirects system app/icon/resource lookup.
The picker returns a volume-qualified path, preserving the selected device
while restoring the original folder. This applies to DOOM WADs and NES ROMs,
including companion files and battery saves. DOOM File/Open reloads the engine.

Every newly compiled native app contains its own 16×16 ICO trailer. A copied
.RUN retains that icon without a sidecar. System-only desktop controls use
icons from the fixed system volume. The Start button has a custom icon;
Start > Restart [R] resets the machine, and Start > Power off [P] requests
ACPI S5. Shell equivalents are REBOOT and POWEROFF. Unsupported ACPI firmware
reports that automatic power off failed rather than claiming it succeeded.

Use the GRUB boot menu to select 640×480, 800×600, 1024×768, 1280×720,
1280×800, 1280×1024, 1366×768, 1440×900, 1600×900, 1920×1080 or 1920×1200.
The firmware must offer the selected 32-bit RGB mode; otherwise GRUB falls
back to 800×600. Settings shows the actual mode. Native apps keep an 800×600
virtual drawing surface and scale when fullscreen. A reboot is required to
change resolution. The persistent desktop canvas occupies reserved RAM at
80–96 MiB, separate from the app, resettable heap and installer payload.

VirtualBox: power off, Settings > Network > Adapter 1 > NAT, Cable Connected,
Intel PRO/1000 MT Desktop (82540EM) or PCnet-FAST III (Am79C973). Boot and run
NET to see the DHCP lease. In QEMU, use e1000, e1000-82544gc, e1000-82545em,
rtl8139 or pcnet with user networking. Physical cards are untested.

The personal installer retains the supplied ROMs. It remains a BIOS/CSM
installer; installing erases the selected target. Existing release checkpoints
and the previous VM disk are retained. See the dated verification report for
the exact binary checks, and INSTALL.md for installation instructions.
