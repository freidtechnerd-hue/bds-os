# Downloaded media test fixtures

These files are kept outside the source tree and public OS installer. They
are optional inputs to `tests/real_media.py`, not bundled game/media assets.

## Big Buck Bunny DVD

- Source: [Internet Archive: Big Buck Bunny](https://archive.org/details/BigBuckBunny).
- Original file: `big-buck-bunny-NTSC.iso`, 8,294,793,216 bytes.
- SHA-256: `48dd35181c275d81788538c4d9e4846cd385e06edddd856fd6763e6d0ca22c96`.
- Attribution: © 2008 Blender Foundation, [Big Buck Bunny](https://www.bigbuckbunny.org/).
- License: Creative Commons Attribution 3.0.

The download was checked against the archive's SHA-1 metadata. It is an
original authored DVD image with VIDEO_TS IFO files and menu VOBs, including
`VTS_01_0.VOB`; the main movie is title 02 (`VTS_02_1.VOB`). BDS selects DVD
titles directly. The October 21 release also tested authored menu navigation,
autoplay and return to menu using this original ISO. CSS encryption, subtitle
overlays and DTS remain unsupported. See the dated release reports for which
tests were repeated against each binary.

## Open Goldberg Variations

Sources: [Internet Archive recording](https://archive.org/details/OpenGoldbergVariations)
and the [Open Goldberg Variations project](https://opengoldbergvariations.org/).
J. S. Bach's Goldberg Variations, performed by Kimiko Ishizaka; CC0/public domain.

| Local download | Bytes | SHA-256 |
| --- | ---: | --- |
| Goldberg-01.flac | 81,564,398 | `ea6b2443f39a017ca9ca7306287a02c0e6df078c711b0a5dbe33adcb2784d89f` |
| Goldberg-02.flac | 34,028,275 | `5c1bd5952691c2ebc8b5bdc0b47158df490ee8579b90977019db7fb59548b7ca` |

The two FLAC downloads were converted using FFmpeg into 44.1 kHz stereo,
signed 16-bit little-endian PCM and arranged as two CD-DA tracks in
`GOLDBERG.BIN` / `GOLDBERG.CUE` (414.9867 seconds total). An ISO9660 data image,
`goldberg-test-data.iso`, carries those files for testing the optical filesystem.
This is an authored test disc from a downloaded recording, not an original
pressed-CD rip. BIN/CUE playback does not prove physical audio-CD extraction.

Example optional guest tests (paths point to your own downloaded fixtures):

```sh
python3 tests/real_media.py --dvd /path/to/big-buck-bunny-NTSC.iso
python3 tests/real_media.py --cd /path/to/goldberg-test-data.iso
```

See the dated release verification report for what passed on the released
binary. Physical optical drives and arbitrary real hardware remain untested.
