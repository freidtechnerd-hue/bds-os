# BDS .RUN format, versions 1 and 2

The 32-byte little-endian header is defined by `RunHeader` in `include/bds.h`.

| Offset | Bytes | Meaning |
| --- | --- | --- |
| 0 | 4 | ASCII `BDSR` |
| 4 | 2 | Version = 2 for new i386 apps; version 1 remains accepted |
| 6 | 2 | Architecture: 1 = 8086 real mode, 2 = 32-bit x86 |
| 8 | 4 | Initialized code/data byte count, excluding header |
| 12 | 4 | Zero-initialized BSS byte count |
| 16 | 4 | Entry offset relative to payload |
| 20 | 4 | Required total payload memory |
| 24 | 4 | Checksum over code/data plus icon trailer (arch 2); zero on arch 1 |
| 28 | 4 | Version 2: icon trailer length, 22–2048 bytes; version 1: zero |

Payload is executable machine code, constants, and initialized data. No scripts or host ELF/PE executable files are accepted. The ELF build product is converted to raw payload and wrapped in this header. There are no relocations or imports in version 1: every binary must be linked for its architecture's specified base.

## Architecture 2

Link code/data at linear 0x00800000. BSS immediately follows the raw initialized bytes. Maximum total memory is 8 MiB. Entry is called with the C cdecl signature `int app_main(BdsApi *api, int argc, char **argv)`. The current function table is API version 4, preserving the complete v1/v2/v3 prefixes. Apps check the version before using appended fields.

Version 2 stores a 16×16, 32-bit BMP-format Windows ICO directly after the
code/data payload. The icon is part of the file/checksum but is never copied
into executable memory or BSS. Exact file size is `32 + code + icon_length`.
The packer automatically selects `assets/icons/APPNAME.ICO`, falling back to
RUN.ICO for future apps. Pass an optional fourth argument to runpack.py for a
custom icon. Embedded icons take priority over sidecar .ICO files. Version-1
binaries can still use sidecars or the system fallback icon.

Checksum starts at zero and accumulates `sum = sum * 31 + byte` modulo 2^32 over the entire payload. Loader checks magic, version, architecture, flags, exact file size, bounded BSS/memory, entry bounds, and checksum before execution. Missing, truncated, incompatible or corrupted files produce errors and return to the shell/desktop. Checksum is an integrity check, not authentication.

Memory is zeroed before loading the payload. Native return reports the application's exit code. `api->exit(code)` uses the assembly loader's saved stack to return early, then heap reset reclaims all process allocations. The loader does not isolate ring-0 code. It does not support nested app execution, threads, an ELF dynamic linker, or concurrent programs.

### API version 2

The exact order/signatures are in `include/bds.h`. The appended fields are:

| Service | Contract |
| --- | --- |
| `pick_file(filter, path, capacity)` | Modal File Manager; semicolon-separated extension filter; 0 selected, -1 canceled/invalid |
| `font(path)` | Load PSF1 font; 0 success, -1 invalid/read error |
| `audio_write(pcm, frames, rate, channels)` | Signed 16-bit PCM; 1/2 channels, 8–96 kHz, up to 8192 frames; returns accepted input frames or -1; caller handles partial acceptance |
| `music(path, loop)` | Play MP3/MIDI/MUS file; 0 success, -1 error |
| `audio_stop()` | Discard queued PCM; music must be stopped separately |
| `audio_available()` | Nonzero when a supported output controller initialized |
| `fullscreen(value)` | 0 windowed, 1 fullscreen, -1 toggle |
| `resource(path, buffer, capacity)` | Read bounded companion data relative to the executing RUN directory, or an absolute path; byte count / -1 |
| `opened_path(buffer, capacity)` | Retrieve File menu selection after KEY_OPEN; 0 success / -1 |
| `music_data(bytes, size, loop)` | Play borrowed in-memory score/MP3, up to 16 MiB; buffer must remain valid until stopped; null stops music |
| `music_volume(value)` | Clamp digital music volume to 0–127 |

Drawing targets an 800×600 virtual surface. `present`/`blit` display it inside
a window by default; Ctrl+F changes its presentation. Apps receive their usual
coordinates in both modes. File menu selection produces `KEY_OPEN` from input
services; apps supporting file replacement consume `opened_path`. Console
printing still targets Terminal. App exit stops audio/music before resetting
the heap. BIN/DLL/DCL remain data; there is no dynamic linking of Windows DLLs.

### API version 3

Appended after the v2 music_volume service:

| Service | Contract |
| --- | --- |
| `net_info(buffer, capacity)` | Writes Ethernet link/IP status; 1 configured, 0 offline/waiting, -1 invalid buffer |
| `http_get(url, buffer, capacity)` | Synchronous HTTP GET; returns raw HTTP response byte count including headers, NUL terminates; accepts http://, IPv4/DNS and ports; buffer 1024–1048576 bytes |

HTTP errors: -1 no Ethernet link, -2 unsupported/invalid URL or arguments,
-3 DNS failure, -4 timeout/transport error, -5 response exceeds capacity,
-6 ESC cancel. The service pumps PIT/input/audio/network while waiting.
DHCP and DNS each wait at most 15 seconds; TCP at most 20 seconds. Kernel
fetch performs one request; the browser handles headers, chunk decoding and
redirects. The connection closes before return and retains no app buffer.
There is no TLS or socket API. Check `api->version >= 3` before access.

### API version 4 and volume paths

`pointer(int *x, int *y)` returns mouse button bits and virtual 800×600 app
coordinates. Outside the app content, or under a host menu, coordinates are
-1 and buttons are zero. `KEY_ADDRESS` is Ctrl+L.

Filesystem handles are opaque integers retaining their originating volume.
Do not use them as metadata indices. File APIs accept `V0:/path`, `V1:/path`,
etc.; volume numbers are assigned at boot and shown by DRIVES. File pickers
return a qualified path and restore the caller's browsing volume/folder.
The executable's volume/root is active while its app runs; system launchers
always locate executables on the boot-selected system volume. Relative
companion resources resolve beside that executable. Existing file arguments
are qualified before entering an app when necessary. Structural filesystem
mutations require reopening handles; there is no POSIX open-file identity.
File/Open in DOOM returns internal status 42 with a pending selection; the
loader unloads and freshly reloads DOOM with the new IWAD.

## Architecture 1

The **whole file including header** is placed at segment 0x2000 offset 0; code begins at offset 32. Assemble with `cpu 8086`, `bits 16`, `org 0`. Entry offset is still relative to payload, so the actual far-call target is 0x2000:(32+entry). DS/ES are 0x2000, SS remains the kernel stack. Use BIOS services or explicitly documented direct hardware, then RETF. Preserve SS/SP and return-frame integrity. There is no function-table ABI on this edition.

The reader limits file size to 60,000 bytes; loader validates architecture, version, initialized byte count and entry bounds. The sample declares a 64 KiB segment requirement, no BSS and no relocations. Legacy BSS allocation, checksums, general memory requirements and untrusted-process cleanup are not implemented. Only the supplied Hello World native binary is verified on the legacy edition.
