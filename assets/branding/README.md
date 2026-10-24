# Supplied BDX theme

The project creator supplied `Dark logo.png`, `White logo_transparent.png`,
`startmenu_logo art.png`, `xp_pack.zip`, TADA, DING, CHORD and the named click
MP3 for use in BDX-OS. Original logos and sounds are retained here; selected
original icon files are in `xp-icons/`. These supplied files are not claimed
as original GPL source artwork. Their underlying rights remain with their
respective owners.

`xp-pack-readme.txt` retains the supplied theme author's distribution and
custom-theme permission. No executable inside the archive is used. The BDX
adaptation is a separate theme, not the unchanged Windows XP theme.

`tools/import_bdx_assets.py` converts selected icons to the native 16×16 ICO
trailer format (32×32 for x64), fits the supplied dark logo into a boot BMP, and converts the
four sound cues to 48 kHz mono PCM WAV. Normal builds use checked-in assets.

Native 16/32-pixel frames are selected from real ICO resources. The supplied
Image and Text File resources are BMPs with `.ico` names and no alpha channel;
the importer keys border-connected white pixels while preserving enclosed
white artwork. The guest honors alpha and AND masks. The boot surround is
black and active window title bars use classic navy `#000080`.

`cloud-generated.png` was generated with the built-in image tool from the
creator's cloud reference. The returned source is 1672×941; the 3840×2160 BMP
is a Lanczos-resampled display asset, not a claim of native 4K source detail.
x64 caches all 3840×2160 pixels; i386 uses the separate 640×360 BMP.

Generation prompt: “Create a high quality desktop wallpaper using the supplied
cloud image as the visual reference. Preserve its bright pale blue sky and
natural soft white cumulus clouds, but reconstruct fine natural cloud detail
without blockiness or pixelation. Output a wide 16:9 3840x2160 4K wallpaper,
full bleed sky only, no horizon, no land, no words, no logos. Calm daylight,
gentle blue negative spaces, believable soft wisps and voluminous cloud edges.
This is for BDX-OS desktop.”
