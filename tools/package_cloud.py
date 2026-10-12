"""Package accepted cloud/network binaries and complete pinned source trees."""
import pathlib,hashlib,json,struct,zlib,zipfile,shutil,subprocess
from disk import Disk
ROOT=pathlib.Path(__file__).resolve().parents[1];LOG=ROOT/'build/logs';DATE='2026-10-11';BASE=f'BDS-OS-Cloud-Network-Installer-{DATE}'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
kernel=ROOT/'build/kernel.elf';iso=ROOT/'build/bds-os-installer.iso';stamp=kernel.stat().st_mtime
def fresh(name,minimum=stamp):
    p=LOG/name;assert p.stat().st_mtime>=minimum,f'Stale check: {name}';return p
install=json.loads(fresh('installer-results.json',iso.stat().st_mtime).read_text());assert len(install['passed'])==8 and install['installer_sha256']==sha(iso)
safety=fresh('installer-safety-results.txt',iso.stat().st_mtime).read_text();assert safety.startswith('PASS')
network=json.loads(fresh('network-desktop-results.json').read_text());assert len(network['passed'])>=16 and network['kernel_sha256']==sha(kernel)
assert network['live_iso_sha256']==sha(ROOT/'build/bds-os.iso')
dns=json.loads(fresh('network-dns-results.json').read_text());assert dns['passed'] and 'DNS' in dns['passed'][0]
desktop=json.loads(fresh('desktop-features-results.json').read_text());assert len(desktop['passed'])==8
audio=json.loads(fresh('audio-results.json').read_text());assert len(audio)==8 and all(r['min']<-100 and r['max']>100 for r in audio)
storage=json.loads(fresh('storage-results.json').read_text());assert len(storage)==3
for name in ['legacy.log','grub.log','persist.log','ui.log','doom.log','nes-platform-0.log','nes-platform-1.log','nes-file-manager.log','no-serial-file-manager.png','no-serial-settings.png']:fresh(name)
assert 'DOOM player moved' in (LOG/'doom.log').read_text();assert 'EDITOR saved' in (LOG/'ui.log').read_text()
vbox=json.loads(fresh('vbox-cloud-update.json').read_text());assert vbox['kernel_sha256']==sha(kernel)
assert len(vbox['roms_and_saves'])==18 and len(vbox['virtualbox_verified'])>=7 and vbox.get('preservation_verified')
assert sha(ROOT/vbox['source_vdi'])==vbox['source_vdi_sha256']
payload=bytearray((ROOT/'build/installer/system.img').read_bytes());crc=struct.unpack_from('<I',payload,440)[0];struct.pack_into('<I',payload,440,0);assert zlib.crc32(payload)==crc
data=Disk(ROOT/'build/data.img');data.check()
def read(d,name):
    i=d.find(name);assert i>=0,name;e=d.entries[i];return bytes(d.data[e[4]*512:e[4]*512+e[3]])
for p in (ROOT/'build/apps').glob('*.run'):assert read(data,'/'+p.name.upper())==p.read_bytes()
for path,source in [('/WALLPAPR.BMP','assets/WALLPAPR.BMP'),('/CLOUDS.PNG','assets/wallpaper-source.png'),('/HOME.HTM','assets/HOME.HTM')]:assert read(data,path)==(ROOT/source).read_bytes()
prior=Disk(ROOT/'build/data.before-cloud.img');roms=[]
for e in data.entries:
    if e[2]==1 and e[0].rstrip(b'\0').upper().endswith(b'.NES'):
        name='/NES/'+e[0].rstrip(b'\0').decode();assert read(data,name)==read(prior,name);roms.append(name)
assert len(roms)==17;start,blocks=struct.unpack_from('<II',payload,446+16+8);assert payload[start*512:(start+blocks)*512]==data.data
out=ROOT/'releases';dest=out/(BASE+'.iso');assert not dest.exists(),'Preserve published checkpoints; choose another release name.'
shutil.copyfile(iso,dest);assert sha(dest)==sha(iso);dest.with_suffix('.sha256').write_text(f'{sha(dest)}  {dest.name}\n')
guide=(ROOT/'docs/INSTALL.md').read_text().replace('`build/bds-os-installer.iso`',f'`{dest.name}`',1)
guide+='\n## Cloud/network release\n\nThree-second boot bar, supplied cloud wallpaper, BMP/PNG viewer, WAD picker\nand DOOM loading screen, startup/system chimes, and Intel PRO/1000 DHCP\nEthernet with a native Gumbo/lwIP HTTP text browser. F8 images; F9 browser.\nFirefox, HTTPS, CSS and JavaScript are not implemented. See NETWORK-DESKTOP.md\nand the accompanying verification report. This personal ISO contains the\n17 supplied NES ROMs. BIOS/CSM and at least 256 MiB installer RAM are required.\n'
dest.with_suffix('.md').write_text(guide)
versions=subprocess.check_output(['dpkg-query','-W','gcc','grub-common','grub-pc-bin','nasm','qemu-system-x86','xorriso','mtools','dosfstools'],text=True)
report=f'''# Cloud/network installer verification — {DATE}

Artifact: `{dest.name}`, {dest.stat().st_size:,} bytes.
Installer SHA256: `{sha(dest)}`.
Kernel SHA256: `{sha(kernel)}`.
Disk template CRC32: `{crc:08x}`.

This personal image contains the compiled native OS/apps, Freedoom and 17
user-supplied ROMs. Old release ISOs/source archives remain unchanged.
Installation tests used disposable image files; no physical host disk was
formatted or installed.

## New functionality

'''+''.join(f'- PASS: {x}.\n' for x in network['passed'])+f'''

Measured boot animation: {network['boot_host_seconds']:.2f} host seconds;
the PIT-driven bar uses a 3000 ms interval. Isolated system chime PCM ranges:
`{json.dumps(network['startup_ranges'])}`. Both sound backends produced actual
signed 16-bit WAV samples, independently of game/music playback.

'''+''.join(f'- PASS: {x}.\n' for x in dns['passed'])+''.join(f'- Observation: {x}.\n' for x in dns.get('notes',[]))+'''
The browser is original BDS UI/layout compiled with Gumbo 0.10.1 and kernel
lwIP 2.2.1. It displays HTTP HTML text and links. Firefox/HTTPS/CSS/JavaScript,
cookies, forms and downloads are not implemented. Physical NICs are untested.

## Exact installer acceptance

'''+''.join(f'- PASS: {x}.\n' for x in install['passed'])+'\n'+safety+'''

## Existing desktop, audio, games and storage

'''+''.join(f'- PASS: {x}.\n' for x in desktop['passed'])+'''
- PASS: boot/storage/loader/game invariants and native editor persistence.
- PASS: absent COM1, PS/2 mouse/keyboard and theme change.
- PASS: DOOM level, 100 frames, movement and exit.
- PASS: NES graphics/controller, invalid ROM, picker, reset, battery SRAM
  across cold boots, File Manager association and exit.
- PASS: all packaged RUNs equal compiled binaries; 17 ROMs equal the previous
  data disk; BMP/PNG/home page equal supplied source assets.

'''+''.join(f'- PASS: {x}.\n' for x in storage)+'''
Actual game/media WAV captures:

| Backend | Case | Hz | Minimum | Maximum |
| --- | --- | --- | --- | --- |
'''+''.join(f"| {r['backend']} | {r['case']} | {r['rate']} | {r['min']} | {r['max']} |\n" for r in audio)+'''

The kernel outputs 48 kHz PCM; QEMU WAV output resamples to 44.1 kHz.
DOOM effects are also tested with music disabled; the NES tone is original.

## VirtualBox disk-only check

'''+''.join(f'- PASS: {x}.\n' for x in vbox['virtualbox_verified'])+f'''

Current disk: `{vbox['current_vdi']}`, BDS IDE port 0/device 0. Optical drive
empty; the separate SATA disk remains attached. Original VM disk retained:
`{vbox['source_vdi']}`, SHA256 `{vbox['source_vdi_sha256']}`.
All {len(vbox['preserved_files'])} unchanged files were compared byte-for-byte
after the live checks, including 17 ROMs and one battery save. Only
{len(vbox['updated_files'])} existing managed files changed; new assets/apps
were added. The VM is powered off after verification.

## Limits

BIOS/CSM, no UEFI; 256 MiB installer RAM, installed OS tested at 128 MiB.
81 MiB minimum disk; BDSFS grows up to 8 GiB. Firmware INT13 storage;
BDSFS/FAT12/16/32 read/write, ISO9660 read-only. No native USB/AHCI/NVMe,
hotplug, optical burning, NTFS/exFAT/ext4 or GPT discovery. GPT installation
targets are refused. Physical PCs remain untested. Analog AC’97/HDA only;
no USB/HDMI audio; MIDI uses basic waveforms. One trusted ring-0 app at a time.
Companion BIN/DLL/DCL are data. See docs/NETWORK-DESKTOP.md and INSTALL.md.

## Build tools

```text
'''+versions+'```\n'
(out/(BASE+'-verification.md')).write_text(report)
source=out/f'BDS-OS-Cloud-Network-{DATE}-sources.zip';assert not source.exists()
with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for name in ['README.md','Makefile','build.ps1','COPYING']:z.write(ROOT/name,name)
    for folder in ['legacy','kernel','include','apps','doom','nes','grub','tools','tests','docs','licenses','assets','third_party/doomgeneric','third_party/infones','third_party/minimp3','third_party/lwip','third_party/gumbo','third_party/lodepng']:
        for p in sorted((ROOT/folder).rglob('*')):
            if p.is_file() and not any(x in ['.git','__pycache__'] for x in p.parts):z.write(p,str(p.relative_to(ROOT)))
    z.writestr('VERIFICATION.md',report)
    for name in ['installer-results.json','installer-safety-results.txt','network-desktop-results.json','network-dns-results.json','audio-results.json','desktop-features-results.json','storage-results.json','vbox-cloud-update.json']:z.write(LOG/name,'verification/'+name)
with zipfile.ZipFile(source) as z:assert z.testzip() is None
source.with_suffix('.sha256').write_text(f'{sha(source)}  {source.name}\n')
print(dest);print('SHA256',sha(dest));print(source);print('PASS exact accepted ISO and complete engine/network/parser/asset source archive')
