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

Press **F6** for Settings. Click a resolution or use the arrow keys, then
click **Apply** or press Enter. The display changes immediately. Click **Keep**
or press Enter again within 15 seconds to save it; Escape, Revert, or the
timeout restores the previous size. Only firmware-supported RGB32 sizes are
listed. The preference is stored in `/DISPLAY.CFG` on the system volume,
independently of the drive currently browsed in File Manager.

Normal desktop and Game Center boots restore the saved size. For recovery,
choose **Desktop 800x600 (ignore saved settings)** in GRUB. Explicit GRUB
resolution entries also override the saved preference. Unsupported requested
boot modes fall back to 800x600. Live switching needs a compatible VBE BIOS;
otherwise Settings explains that the boot menu must be used. Physical GPU and
monitor combinations remain untested. The preview timer requires the kernel
to remain running; reboot to recovery if firmware hangs during a mode change.

Native apps keep an 800x600 virtual drawing surface and scale proportionally when fullscreen, with centered borders.
See [the tabbed Settings guide](SETTINGS.md) for modern modes and preferences.
The persistent desktop canvas occupies reserved RAM at 80-112 MiB, separate
from apps, the resettable heap and installer payload. No GPU acceleration or
monitor EDID filtering is implemented.

VirtualBox: power off, Settings > Network > Adapter 1 > NAT, Cable Connected,
Intel PRO/1000 MT Desktop (82540EM) or PCnet-FAST III (Am79C973). Boot and run
NET to see the DHCP lease. In QEMU, use e1000, e1000-82544gc, e1000-82545em,
rtl8139 or pcnet with user networking. Physical cards are untested.

The personal installer retains the supplied ROMs. It remains a BIOS/CSM
installer; installing erases the selected target. Existing release checkpoints
and the previous VM disk are retained. See the dated verification report for
the exact binary checks, and INSTALL.md for installation instructions.
