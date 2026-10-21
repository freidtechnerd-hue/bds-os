# Native HTTPS browser update

The browser now follows HTTP and HTTPS URLs/redirects and can load up to 2 MiB
per page. The native Gumbo HTML5 parser and bounded Elk scripting engine remain
in use. Wikipedia's English Main Page and example.com were fetched and rendered
inside QEMU. Wikipedia currently appears as a simplified text page with links,
not the full PC layout. CSS layout, images, Unicode fonts and JavaScript APIs
remain incomplete; this is not Firefox or a replacement for a modern browser.
Compressed responses are not decoded; requests explicitly ask for identity
encoding. CSS/scripts are bounded and limited to the same origin.

## TLS and requirements

BearSSL is compiled into the kernel and connected directly to lwIP TCP. TLS 1.2
uses ECDHE with AES-GCM or ChaCha20-Poly1305. Certificate chains, DNS hostname
and validity dates are checked against the bundled Mozilla/curl roots. The
hardware clock must contain a valid UTC date. There is no ignore-certificate
option, plaintext fallback, host rendering proxy, cookies or credential store.
Wrong-host, self-signed and expired certificates were rejected in VM tests.
This release does not implement TLS 1.3, certificate revocation fetching or
Firefox's additional trust-store policy constraints.

Secure entropy currently requires CPU **RDRAND**. Without that capability,
HTTPS returns an explicit error while HTTP continues to work. For QEMU use
`-cpu max` (or an appropriate host CPU with hardware virtualization), `-vga std`
and an Ethernet adapter such as `-nic user,model=e1000`. VirtualBox must expose
RDRAND from the host CPU. This has not been validated on every physical PC or
VirtualBox host. Older processors without RDRAND are not yet supported for TLS.

The CA bundle is pinned in `third_party/ca-certificates`, with its source URL,
hash and MPL-2.0 license. BearSSL's commit and MIT license are retained in
`third_party/bearssl`. `kernel/tls_anchors.h` is generated from that PEM with the
upstream host `brssl ta` tool. Rebuild it when updating trust roots; changing the
PEM alone does not update the compiled trust store. A root-store update requires
rebuilding the kernel and installer.

Tests: `python3 tests/https_internet.py` requires live Internet; previous HTTP,
redirect, chunked-body and DOM/CSS tests remain under `tests/network_desktop.py`
and `tests/desktop_upgrade.py`.
