# TrueType and desktop fonts

Settings → Fonts → **Choose TrueType [T]** opens File Manager for `.TTF` files.
Choose a file on any mounted readable volume. A successful selection is copied
to `/FONTS/USER.TTF` on the system volume and restored on the next boot. A
read-only system volume can apply a font for the current session only.

The public images include DejaVu Sans Mono with its redistribution license.
You may supply Arial (`arial.ttf`), Times New Roman (`times.ttf`), Comic Sans
(`comic.ttf`) or other TrueType fonts from your own installation. Microsoft
fonts used in local verification are not bundled in the repository or ISOs.

Terminal command: `FONT /FONTS/DEJAVU.TTF`. Native apps expose the same loader
in Options → Load PSF/TrueType font. `FONT DEFAULT` restores the bitmap font;
Settings → Smooth text restores the built-in antialiased DejaVu atlas.

FreeType 2.14.3 rasterizes the file inside the native kernel. Files are limited
to 2 MiB; invalid fonts leave the previous font active. The current desktop
retains eight-pixel character columns and rasterizes printable ASCII into
8×12 cells. Wide glyphs are fitted into those cells. This is not yet full
proportional layout, Unicode shaping, OpenType/CFF, emoji or arbitrary font
size support. PSF1 remains available for the classic 8×8 appearance.

`tests/bdx_theme.py` checks DejaVu and, when present on the test host, Arial,
Times New Roman and Comic Sans; malformed input rejection, the picker, and
cold-boot persistence are also exercised. FreeType's source and FTL/GPLv2
license alternatives are retained under `third_party/freetype`.
