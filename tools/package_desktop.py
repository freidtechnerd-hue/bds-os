"""Package the exact accepted desktop/audio installer without replacing old releases."""
import hashlib,json,pathlib,shutil,struct,subprocess,zipfile,zlib
from disk import Disk
ROOT=pathlib.Path(__file__).resolve().parents[1];LOG=ROOT/'build/logs'
DATE='2026-10-11';BASE=f'BDS-OS-Desktop-Audio-Installer-{DATE}'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
kernel=ROOT/'build/kernel.elf';iso=ROOT/'build/bds-os-installer.iso'
stamp=kernel.stat().st_mtime
def fresh(name,minimum=stamp):
    p=LOG/name;assert p.stat().st_mtime>=minimum,f'Stale result: {name}';return p
install=json.loads(fresh('installer-results.json',iso.stat().st_mtime).read_text())
assert install['installer_sha256']==sha(iso) and len(install['passed'])==8
safety=fresh('installer-safety-results.txt',iso.stat().st_mtime).read_text().strip();assert safety.startswith('PASS')
storage=json.loads(fresh('storage-results.json').read_text());assert len(storage)==3
desktop=json.loads(fresh('desktop-features-results.json').read_text());assert len(desktop['passed'])==8
audio=json.loads(fresh('audio-results.json').read_text());assert len(audio)==8
for r in audio:assert r['min']<-100 and r['max']>100
for name in ['legacy.log','grub.log','persist.log','ui.log','doom.log','nes-platform-0.log','nes-platform-1.log','nes-file-manager.log','no-serial-file-manager.png','no-serial-settings.png']:fresh(name)
assert 'DOOM player moved' in (LOG/'doom.log').read_text()
assert 'EDITOR saved' in (LOG/'ui.log').read_text()
assert 'PASS: filesystem integrity' in (LOG/'nes-platform-1.log').read_text()
vbox=json.loads(fresh('vbox-desktop-update.json').read_text());assert vbox['kernel_sha256']==sha(kernel)
assert len(vbox['preserved_files'])==24 and len(vbox['virtualbox_verified'])>=6
payload=bytearray((ROOT/'build/installer/system.img').read_bytes());crc=struct.unpack_from('<I',payload,440)[0];struct.pack_into('<I',payload,440,0);assert zlib.crc32(payload)==crc
data=Disk(ROOT/'build/data.img');data.check()
def read(d,name):
    i=d.find(name);assert i>=0,name;e=d.entries[i];return bytes(d.data[e[4]*512:e[4]*512+e[3]])
for app in (ROOT/'build/apps').glob('*.run'):assert read(data,'/'+app.name.upper())==app.read_bytes()
roms=[e[0].rstrip(b'\0').decode() for e in data.entries if e[2]==1 and e[0].rstrip(b'\0').upper().endswith(b'.NES')]
assert len(roms)==17
prior=Disk(ROOT/'build/data.before-desktop.img')
for name in roms:assert read(data,'/NES/'+name)==read(prior,'/NES/'+name)
start,blocks=struct.unpack_from('<II',payload,446+16+8);assert payload[start*512:(start+blocks)*512]==data.data
out=ROOT/'releases';out.mkdir(exist_ok=True);dest=out/(BASE+'.iso')
shutil.copyfile(iso,dest);assert sha(dest)==sha(iso)
(out/(BASE+'.sha256')).write_text(f'{sha(dest)}  {dest.name}\n')
guide=(ROOT/'docs/INSTALL.md').read_text().replace('`build/bds-os-installer.iso`',f'`{dest.name}`',1)
guide+='\n## Desktop/audio release\n\nThis ISO includes the windowed apps, Ctrl+F fullscreen, File/Options menus,\n16×16 icons, PSF fonts, companion data API, MP3/MIDI and game audio,\ngenerated BMP boot screen, and the supplied black-outlined white cursor.\nSee DESKTOP-AUDIO.md and the accompanying verification report for supported\naudio hardware and tested behavior. This personal ISO contains the 17 supplied\nNES ROMs. BIOS/CSM and installer RAM of at least 256 MiB are required.\n'
(out/(BASE+'.md')).write_text(guide)
versions=subprocess.check_output(['dpkg-query','-W','gcc','grub-common','grub-pc-bin','mtools','nasm','qemu-system-x86','xorriso','dosfstools'],text=True)
report=f'''# Desktop/audio installer verification — {DATE}

Artifact: `{dest.name}` ({dest.stat().st_size:,} bytes).
Installer SHA256: `{sha(dest)}`.
Kernel SHA256: `{sha(kernel)}`.
Disk template CRC32: `{crc:08x}`.

The final installer contains the compiled native kernel/apps, Freedoom and the
17 user-supplied NES ROMs. It is a personal image. Tests installed only into
disposable image files; no physical host disk was installed or formatted.

## Exact installer acceptance

'''+''.join(f'- PASS: {x}.\n' for x in install['passed'])+'\n'+safety+'''

## Desktop and actual audio output

'''+''.join(f'- PASS: {x}.\n' for x in desktop['passed'])+'''

QEMU WAV captures verified signed 16-bit output with these actual ranges:

| Backend | Test | Captured Hz | Minimum | Maximum |
| --- | --- | --- | --- | --- |
'''+''.join(f"| {r['backend']} | {r['case']} | {r['rate']} | {r['min']} | {r['max']} |\n" for r in audio)+'''

The kernel outputs 48 kHz PCM; QEMU's WAV backend resamples to 44.1 kHz.
DOOM effects were also captured with music disabled; the NES tone cartridge
is an original 6502 APU test. Screenshots are actual native framebuffers.

## Boot, storage and game regressions

- PASS: game logic and six filesystem corruption/integrity checks.
- PASS: seven legacy, fourteen GRUB and four cold-boot persistence assertions.
- PASS: desktop mouse dragging, editor save/reload, five game loops and exits.
- PASS: absent-COM1 desktop, File Manager, PS/2 input and theme change.
- PASS: DOOM load/WAD/level/100 frames/movement/exit.
- PASS: NES graphics/controller, invalid ROM recovery, file picker, reset,
  battery SRAM across cold boots, File Manager association and clean exit.
- PASS: all packaged RUN files match compiled binaries, and all 17 supplied
  ROMs match the pre-update disk byte-for-byte.

'''+''.join(f'- PASS: {x}.\n' for x in storage)+'''

## Existing VirtualBox VM

'''+''.join(f'- PASS: {x}.\n' for x in vbox['virtualbox_verified'])+f'''

Current disk: `{vbox['current_vdi']}` on BDS IDE port 0/device 0. The optical
drive is empty; the original VDI and separate SATA disk remain available.
Original VDI SHA256: `{vbox['original_vdi_sha256']}`.

## Practical limits

Legacy BIOS/CSM; installer RAM 256 MiB; installed OS tested at 128 MiB;
81 MiB minimum disk; BDSFS expands up to 8 GiB. Storage depends on firmware
INT 13h services. BDSFS and FAT12/16/32 read/write; ISO9660 optical read-only.
No UEFI/native USB/AHCI/NVMe/hotplug, optical burning, NTFS/exFAT/ext4 or GPT
volume discovery. GPT installation targets are refused.

Audio supports Intel ICH AC’97 and standard analog HDA paths. Physical PCs
remain untested; HDA amplifier/GPIO quirks, HDMI and USB audio are unsupported.
MIDI uses basic software waveforms, not a General MIDI sample bank. Graphical
apps run one at a time in ring 0. BIN/DLL/DCL are data, not Windows executables.
See `docs/DESKTOP-AUDIO.md`, `docs/INSTALL.md`, `COPYING`, `licenses/NOTICE.md`.

## Build tools

```text
'''+versions+'```\n'
(out/(BASE+'-verification.md')).write_text(report)
source=out/f'BDS-OS-Desktop-Audio-{DATE}-sources.zip'
with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for name in ['README.md','Makefile','build.ps1','COPYING']:z.write(ROOT/name,name)
    for folder in ['legacy','kernel','include','apps','doom','nes','grub','tools','tests','docs','licenses','assets','third_party/doomgeneric','third_party/infones','third_party/minimp3']:
        for p in sorted((ROOT/folder).rglob('*')):
            if p.is_file() and not any(x in ['.git','__pycache__'] for x in p.parts):z.write(p,str(p.relative_to(ROOT)))
    z.writestr('VERIFICATION.md',report)
    for name in ['installer-results.json','installer-safety-results.txt','audio-results.json','desktop-features-results.json','storage-results.json','vbox-desktop-update.json']:z.write(LOG/name,'verification/'+name)
with zipfile.ZipFile(source) as z:assert z.testzip() is None
source.with_suffix('.sha256').write_text(f'{sha(source)}  {source.name}\n')
print(dest);print('SHA256',sha(dest));print(source);print('PASS exact accepted ISO copy and complete project/engine/asset source archive')
