> October 21: native certificate-validated HTTPS is now available. See
> [HTTPS requirements and limitations](HTTPS.md). Wikipedia loads in simplified
> text layout; full PC rendering compatibility has not been implemented.

# Cloud desktop, images and Ethernet

The supplied 200×150 cloud image is the default wallpaper. `/WALLPAPR.BMP`
is cached in a compact static buffer and stretched to the display; it survives
app heap resets. The original PNG is `/CLOUDS.PNG`. Settings 1 selects clouds,
2 selects amber. The boot illustration stays visible for three seconds with
a progress bar. AC’97/HDA hardware plays an original startup chime; opening
desktop windows and launch errors have sounds. Options/Mute controls the
system/app mixer. No supported sound controller means silent operation.

## Image Viewer

F8, Start/8 or Image Viewer opens a picker. File Manager associates BMP/PNG
with `/IMAGE.RUN`. `IMAGE /CLOUDS.PNG` opens directly. O or File/Open selects
another image; Ctrl+F toggles fullscreen; ESC/F10 exits. Images fit the window
while preserving aspect ratio. Files remain unmodified.

Formats: uncompressed 24/32-bit Windows BMP and PNG, including palette/alpha
PNG through LodePNG. PNG transparency composites over a checkerboard. Limits:
8 MiB input, 1920×1080 pixels, 9 MiB inflated PNG data. JPEG/GIF, animation and
editing are not implemented. Damaged files show an error in the viewer.

## DOOM

Game Center/D or `DOOM` opens the WAD picker with Freedoom highlighted. Choose
a compatible **IWAD** containing the base game; Enter starts it, ESC cancels.
File Manager opens `.WAD` with DOOM; `DOOM /MYGAME.WAD` opens directly.
Additional PWADs use the engine command line:
`RUN /DOOM.RUN -iwad /FREEDOOM1.WAD -file /EXTRA.WAD`.

The native window appears before engine initialization and shows the selected
WAD, loading status and progress while the large cached WAD is read. The screen
remains visible during texture/sound preparation until the game draws. BDSFS
reads contiguous aligned data in blocks up to 32 KiB, retaining partial-sector
and end-of-file behavior. Initialization time still depends on hardware/WAD.
Use the picker's visible < / > drive buttons, Tab or Left/Right to change
devices. The selected path retains its drive, and returning restores the
previous folder and drive. File/Open inside DOOM selects another IWAD and
restarts the engine with that game. F10 exits; Ctrl+F toggles fullscreen.

## Ethernet and BDS Browser

F9, Start/9 or Web Browser launches `/BROWSER.RUN`. `WEB` opens its local home;
`WEB http://host/path` loads a URL. `NET` reports Ethernet status. The driver
supports Intel 82540EM/82544GC/82545EM (8086:100e/100c/100f), Realtek RTL8139
(10ec:8139, legacy mode), and AMD PCnet (1022:2000, Am79C970A/973). The Intel
driver requires a 32-bit memory BAR; Realtek/AMD require an I/O BAR.
In VirtualBox, power off the VM, open Settings > Network > Adapter 1, enable
the adapter, select NAT and check Cable Connected. Under Advanced, choose
Intel PRO/1000 MT Desktop (82540EM) or PCnet-FAST III (Am79C973). Boot BDS and
run NET; a DHCP address and gateway indicate configuration. No port-forward
rule is needed for outgoing browsing. For QEMU:

```sh
qemu-system-i386 -m 128 -cdrom build/bds-os.iso \
  -drive file=build/data.img,format=raw,if=ide,index=0 \
  -netdev user,id=n -device e1000,netdev=n
```

Polled DMA rings feed lwIP 2.2.1 in NO_SYS mode. DHCP supplies IPv4 address,
netmask, gateway and DNS. ARP, ICMP, UDP, DNS and TCP run in the cooperative
main context without sockets/POSIX host services. Static network buffers
survive app heap resets. Link changes are detected and DHCP follows them.
Use `rtl8139`, `pcnet`, `e1000-82544gc` or `e1000-82545em` in place of
`e1000` to exercise those adapters. Only one supported NIC is initialized.
Wi-Fi, dial-up/USB modems, virtio-net, IPv6 and manual IP configuration are not implemented.
Physical cards remain untested.

BDS Browser is compiled with Google's open-source Gumbo 0.10.1 HTML5 parser
and the kernel's lwIP transport. The window, layout and navigation are original
BDS code. **Firefox was not compiled.** It needs process isolation, virtual
memory, threads, POSIX interfaces and many libraries this kernel lacks.

Click the white address field and type a URL, then Go or Enter loads it.
Ctrl+L, L or F2 also focus the address. Bare hostnames gain an http:// prefix.
Typing a URL directly starts address entry. Tab selects links; Enter follows;
clicking a text line containing a link follows its first link. Up/Down and
Space scroll; R reloads; uppercase H returns home. O or File/Open opens local
HTM/HTML/TXT. File Manager associates HTML files with the browser. Ctrl+F
switches fullscreen. ESC exits or cancels address entry/network fetch.

HTTP/1.x GET, IPv4 literals/domains, explicit ports, up to five requests in a
redirect chain, chunked responses, HTML entities and relative links are
supported. Limits: 248-character entered URL, 2 MiB response, 4096 text lines,
64 links, 128 rendering levels. Pictures show alternate text. Status reports
HTTP errors, truncation and network failures. DHCP/DNS waits cap at 15 seconds
each; the TCP request caps at 20 seconds. A new navigation clears the old
document, so an error cannot leave the home page looking like the result.

CSS supports inline style, style blocks and same-origin linked stylesheets:
tag, .class, #id, compound simple selectors, comma lists, specificity/source
order, inherited text color, backgrounds, bold, alignment, display:none,
preformatted whitespace and left margin. Named colors and #RGB/#RRGGBB work.
This remains fixed-cell text layout; flex/grid, fonts/sizes, positioning,
complex selectors and full browser geometry are not implemented.

Elk 3.0 executes a JavaScript subset: `let`, arithmetic, strings, conditions,
`for`, `break`/`continue`, function expressions and calls. The DOM bridge has
`document.title`, `document.write`/`writeln`, `getElementById(...).textContent`,
`innerHTML`, and element style color/backgroundColor/display. Console.log
writes to Terminal. Inline and same-origin external scripts run at load,
followed by body onload; document.write output is appended after document
content. There is no event loop, click-handler DOM, timer, fetch, storage or
native filesystem binding. Elk does not implement the complete JS language
(including var, const, while, arrays, classes and browser standard libraries).
Numeric output uses up to six fractional digits. Limits per document: eight
external resources of at most 16 KiB, sixteen scripts, 256 KiB interpreter
memory, 20,000 execution steps per script, 16 nested function calls, 48 DOM
references, 8 KiB written HTML. Script errors leave navigation usable.

Simple HTTP sites, including many early-web pages, can display text and links
with this subset. A site being from the 1990s does not guarantee compatibility:
its current server may require HTTPS or features outside this browser.
Forms, cookies, downloads, compression and history remain
unsupported. Apps/parsers run at kernel privilege without process isolation.

## Pinned source and specifications

- [lwIP](https://github.com/lwip-tcpip/lwip), 2.2.1,
  `77dcd25a72509eb83f72b033d219b1d40cd8eb95`, BSD three-clause.
  [Main-loop documentation](https://www.nongnu.org/lwip/2_1_x/group__lwip__nosys.html).
- [Gumbo](https://github.com/google/gumbo-parser), 0.10.1,
  `3973c58d759574f2899528d2b3379e17d66dbcad`, Apache 2.0.
- [LodePNG](https://github.com/lvandeve/lodepng),
  `e584a8490db1b76b81bf298c11c09d3b55d9c270`, zlib license.
  Upstream .cpp was renamed .c for its supported C build, otherwise unchanged.
- [Intel 8254x developer manual](https://www.intel.com/content/dam/doc/manual/pci-pci-x-family-gbe-controllers-software-dev-manual.pdf).
- [Elk](https://github.com/cesanta/elk), commit
  `71a86fa2fef146696be9ae66715bf3f91d0a5f2c`, AGPL-3.0; BDS budget,
  property-access and numeric-shim changes are marked in the source.
- [AMD PCnet-PCI II data sheet](https://www.amd.com/content/dam/amd/en/documents/archived-tech-docs/datasheets/19436.pdf).
- [Realtek programming guide](https://www.cs.usfca.edu/~cruse/cs326f04/RTL8139_ProgrammersGuide.pdf).

Sources/licenses are under `third_party/`. BDS integrations are in
`kernel/network.c`, `kernel/network/include/`, `apps/browser.c`, `apps/image.c`
and `tools/build_utilities.py`. `make test-network` exercises native QEMU,
HTTP fixtures and actual AC’97/HDA WAV captures. The dated release report
records checks performed on the delivered binaries.
