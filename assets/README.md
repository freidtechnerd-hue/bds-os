# Asset sources

`cursor-reference.png` is the user-supplied pointer reference. Its 12×19 pixel
grid is reproduced directly in `kernel/assets.c` as a code-native cursor:
black outline, white interior and transparent exterior, with a top-left
hotspot. No generated variant is used for the pointer.

`BOOT.BMP` is an 800×600 RGB BMP converted from `boot-source.png`. The source
was generated with the ImageGen tool in **generate** mode using this prompt:

> Use case: stylized-concept. Asset type: boot splash background for the BDS-OS hobby operating system, intended for 800x600 display. Create a polished retro computing illustration in a 4:3 landscape composition: deep forest green and dark charcoal backdrop, a softly lit vintage desktop computer centered, subtle pixel-grid accents, warm cream highlights, restrained amber glow. Keep the lower quarter quiet and dark for a runtime progress bar and status text. The central monitor shows the exact short text "BDS-OS" in clear block lettering. No other words, no trademarks, no watermark. Clean silhouettes and readable contrast at small resolution. This is a complete full-screen bitmap illustration, no frames or web page.

`tools/desktop_assets.py` creates the original 16×16 ICO files, PSF fonts and
original MIDI melody. Fonts derive from the public-domain `kernel/font8x8.h`.
`tools/demo_mp3.py` creates the original MP3 melody using the build host's LAME
encoder; LAME is not linked into the kernel or shipped as an OS component.
The saved MP3 does not require the encoder to rebuild the OS.

The cloud wallpaper is the user-supplied PNG, preserved as wallpaper-source.png.
WALLPAPR.BMP is its 200x150 RGB BMP conversion without visual edits. The
startup/window/error sounds are original triangle-wave chimes synthesized
in kernel/sfx.c; no third-party system sound samples are included.
