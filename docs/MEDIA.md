# Media Player

Open **Media Player** on the desktop, or press **F10, M**. Terminal commands
`MEDIA` and `PLAY` automatically start a mounted DVD or audio CD, otherwise open the file picker; `PLAY V1:/MOVIES/FILM.VOB` opens a file
on another volume. The player requires an SSE2 CPU and API version 6.

| Media | Implemented support |
| --- | --- |
| Music | MP3, MIDI/MUS through the existing software audio engine; 16 MiB file limit |
| WAV | Streaming PCM, 8/16-bit, mono/stereo, 8–96 kHz |
| MPEG video | MPEG-1/2 program streams (.MPG/.MPEG/.VOB), or elementary .M2V video; 4:2:0 chroma, maximum 1024×768 source |
| DVD audio | MPEG Layer II, AC-3 stereo downmix, 16-bit mono/stereo DVD LPCM at 48/96 kHz |
| DVD titles | Unencrypted VIDEO_TS/VTS_nn_1.VOB titles on mounted ISO9660 volumes, with subsequent numbered VOB files |
| CD images | One BINARY .BIN file described by a .CUE with INDEX 01 audio tracks; CD-DA PCM is little-endian stereo 44.1 kHz |
| Physical audio CD | Experimental read-only ATAPI/MMC READ TOC and READ CD on legacy IDE ports; physical drive playback has not been verified |

DVD decoding uses libmpeg2, liba52 and the MP2 decoder from pl_mpeg, compiled
into the native RUN file. Files are streamed with bounded buffers. No host
FFmpeg, Windows codec or Linux multimedia runtime runs inside BDS-OS.

**DVD limitations:** no CSS decryption, subtitle/SPU overlays, angle selection,
DTS, multichannel PCM or alternate audio-track selection. The first supported
audio stream is selected. UDF-only discs cannot be mounted; use an ISO9660
bridge disc or copy unencrypted VOB files onto a readable volume. IFO discs use authored navigation and menus; VOB-only title selection uses
filenames and switching their parts can produce a short gap. MP4/H.264, WebM, streaming websites and Blu-ray are not
supported. Decoder software runs on the CPU; slow emulation can stutter.

## Controls

| Action | Key or button |
| --- | --- |
| Open file | O / Open / File → Open |
| List DVD titles on mounted volumes | D / DVD |
| Read an audio CD | C / CD |
| List compatible files in the current directory | Playlist |
| Play/pause | Space / Play/Pause |
| Stop and return to beginning | S / Stop |
| Next/previous track or listed file | N/P / Next/Prev |
| Seek | Left/Right: 10 seconds for WAV/CD; approximate 5% file position for MPEG |
| Repeat | L / Repeat |
| Volume | + / - |
| Fullscreen | Ctrl+F |
| Exit | Escape or F10 (Escape first closes a playlist) |

Opening a playlist pauses current playback. After closing it, press Play to
resume. MP3/MIDI seeking is not implemented; Stop restarts from the beginning.
MPEG seeking resumes at the next decodable sequence/keyframe and its time
counter restarts. Duration is an estimate for video and unknown for music;
the timeline is a display, not a drag-to-seek control. Fullscreen preserves
the video's display aspect ratio, including anamorphic 16:9 DVD video.

## Drives and sound

Attach media before starting the guest. Ordinary DVD files use BIOS-visible
storage and ISO9660. Use an **IDE optical drive in VirtualBox**: the tested
6.1.50 BIOS did not expose a non-bootable data disc on its SATA optical port.
ISO9660 keeps a bounded 128-entry snapshot and prioritizes VIDEO_TS before
bonus-data directories on hybrid movie DVDs. Physical audio tracks use a separate legacy IDE ATAPI
path because they are not filesystem files. Native AHCI, USB optical drives,
hotplug and host-drive passthrough are not tested/supported audio-CD paths.
A normal ISO represents a data disc and cannot substitute for a music CD;
use BIN/CUE for repeatable audio-CD playback in a VM.

Select **ICH AC97** or **Intel HD Audio** in your VM and enable host audio
output. `AUDIO` in Terminal reports the detected device. Video can play
silently; audio-only media report an error if no supported output initialized.
The existing driver supports analog output; arbitrary real sound cards,
HDMI and USB audio are not supported.

## Widescreen

Press **F6**, select a firmware-provided resolution, **Apply**, then **Keep**
within 15 seconds. Available 16:9 options include 1280×720, 1360×768,
1366×768, 1600×900 and 1920×1080; 1280×800, 1440×900 and 1920×1200 are 16:10.
Firmware determines which sizes appear. The 1360/1366 choices are approximate
16:9 ratios. Auto detects the monitor preferred EDID mode; see [Settings](SETTINGS.md). There is no arbitrary host-window resizing.

For VirtualBox with the VM powered off, extra VBE modes can be configured:

```powershell
VBoxManage setextradata "BDS-OS" "CustomVideoMode1" "1280x720x32"
VBoxManage setextradata "BDS-OS" "CustomVideoMode2" "1600x900x32"
VBoxManage setextradata "BDS-OS" "CustomVideoMode3" "1920x1080x32"
```

Use your actual VM name and VBoxManage installation path. The OS still
validates each reported mode. If a mode fails, let the preview revert; GRUB's
800×600 recovery entry ignores saved preferences.

## Rebuilding and testing

`make media` builds the player from vendored source. `make test-media` needs
QEMU and a host FFmpeg executable (`FFMPEG=/path/to/ffmpeg` overrides its
location). Tests generate original tone/color fixtures, decode actual guest
pictures and inspect captured PCM. No commercial disc contents are bundled.
See the [verification record](VERIFIED.md) for the checks actually completed.

## DVD menus and autoplay (October 21 update)

The player compiles VideoLAN libdvdnav 7.0.0 and libdvdread 7.1.1's navigation
and IFO parsers with a native read-only BDS adapter. **DVD / D** starts the first
mounted disc containing `VIDEO_TS/VIDEO_TS.IFO`; an IFO can also be selected in
Open. Authored first-play instructions run automatically, including introduction
video and menus. **Arrows** select a menu button, **Enter** activates it, and
**M** returns to the root menu. Clicking a menu button activates it. A yellow
outline shows the active button. Previous/Next use DVD program navigation.
VOB-only media retain the title/file picker fallback.

Autoplay runs when Media Player opens or DVD/CD is selected. The desktop does
not launch an application on insertion, and hotplug/remount support remains
limited by the BIOS/optical driver. DVD subtitles/SPU overlay graphics and exact
authored highlight palettes are not rendered. Menus whose button labels exist
only in SPU overlays may therefore be incomplete. CSS-encrypted and UDF-only
media remain unsupported. The freely licensed Big Buck Bunny NTSC disc is the
real-disc menu test; this is not a claim of compatibility with every DVD.
