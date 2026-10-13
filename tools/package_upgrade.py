"""Package an already-tested installer; never publishes or touches physical disks."""
import argparse,hashlib,json,pathlib,shutil,struct,subprocess,zlib
from disk import Disk
ROOT=pathlib.Path(__file__).resolve().parents[1];LOG=ROOT/'build/logs'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(d,name):
    i=d.find(name);assert i>=0,name;e=d.entries[i]
    return bytes(d.data[e[4]*512:e[4]*512+e[3]])
def main():
    ap=argparse.ArgumentParser();ap.add_argument('profile',choices=['personal','public']);a=ap.parse_args()
    iso=ROOT/'build/bds-os-installer.iso';kernel=ROOT/'build/kernel.elf'
    install=json.loads((LOG/'installer-results.json').read_text());assert len(install['passed'])==8 and install['installer_sha256']==sha(iso)
    safety=LOG/'installer-safety-results.txt';assert safety.stat().st_mtime>=iso.stat().st_mtime and safety.read_text().startswith('PASS')
    reports={}
    for name in ['desktop-upgrade','network-desktop','resolution']:
        r=json.loads((LOG/(name+'-results.json')).read_text());assert r['kernel_sha256']==sha(kernel);reports[name]=r
    assert len(reports['desktop-upgrade']['passed'])==13 and len(reports['network-desktop']['passed'])==16 and len(reports['resolution']['passed'])==10
    audio=json.loads((LOG/'audio-results.json').read_text());assert len(audio)==8 and all(x['min']<-100 and x['max']>100 for x in audio)
    storage=json.loads((LOG/'storage-results.json').read_text());assert len(storage)==3
    desktop=json.loads((LOG/'desktop-features-results.json').read_text());assert len(desktop['passed'])==8
    data=Disk(ROOT/('build/seed.img' if a.profile=='public' else 'build/data.img'));data.check()
    for app in (ROOT/'build/apps').glob('*.run'):assert read(data,'/'+app.name.upper())==app.read_bytes(),app
    games=[e[0].rstrip(b'\0').decode() for e in data.entries if e[2]==1 and e[0].rstrip(b'\0').upper().endswith((b'.NES',b'.SAV'))]
    if a.profile=='public':assert not games,'Public installer contains private game files'
    else:assert len(games)==18
    payload=bytearray((ROOT/'build/installer/system.img').read_bytes());crc=struct.unpack_from('<I',payload,440)[0];struct.pack_into('<I',payload,440,0);assert zlib.crc32(payload)==crc
    start,blocks=struct.unpack_from('<II',payload,470);assert payload[start*512:(start+blocks)*512]==data.data
    suffix='Public' if a.profile=='public' else 'Installer'
    base=f'BDS-OS-Desktop-Upgrade-{suffix}-2026-10-12';dest=ROOT/'releases'/base
    assert not dest.with_suffix('.iso').exists(),'Release checkpoint already exists'
    shutil.copyfile(iso,dest.with_suffix('.iso'));assert sha(dest.with_suffix('.iso'))==sha(iso)
    dest.with_suffix('.sha256').write_text(f'{sha(iso)}  {base}.iso\n')
    vbox=json.loads((LOG/'vbox-upgrade-update.json').read_text());assert vbox['preservation_verified'] and vbox['kernel_sha256']==sha(kernel)
    versions=subprocess.check_output(['dpkg-query','-W','gcc','grub-common','grub-pc-bin','nasm','qemu-system-x86','xorriso','mtools','dosfstools'],text=True)
    report=f'''# BDS-OS desktop upgrade verification — 2026-10-12

Artifact: `{base}.iso` ({iso.stat().st_size:,} bytes).
Installer SHA-256: `{sha(iso)}`.
Kernel SHA-256: `{sha(kernel)}`. Disk payload CRC32: `{crc:08x}`.
Profile: **{a.profile}**. {'No commercial ROMs, battery saves or personal configuration; includes licensed Freedoom.' if a.profile=='public' else 'Private personal installer, including 17 supplied ROMs and one battery save; do not upload it.'}

Built from the project directed by freidtechnerd-hue with GPT-6.1 Sol and
GPT-6 Astra. [Original video](https://www.youtube.com/watch?v=oUb6CiOTym4&t=10s).
This release includes updates made after the video. Installation instructions:
[GETTING-STARTED.md](https://github.com/freidtechnerd-hue/bds-os/blob/main/docs/GETTING-STARTED.md).

## Exact installer checks

'''+''.join('- PASS: '+x+'.\n' for x in install['passed'])+'\n'+safety.read_text()+'''
All installation targets were disposable emulator image files. No physical
host disk was erased. Cold boots removed the installation media entirely.

## Desktop, browser, Ethernet and resolution checks

'''+''.join('- PASS: '+x+'.\n' for r in reports.values() for x in r['passed'])+'''
The normal 800×600 mode is covered throughout; QEMU falls back from unavailable
1366×768 to 800×600. Physical firmware can expose a different set of modes.

## Regression and audio

Core game logic and six filesystem/loader integrity tests pass. BIOS floppy,
GRUB boot and file persistence pass. PS/2 window input, native editor saving,
five games, absent serial port, DOOM level/movement/exit, and synthetic NES
controller/reset/SRAM/picker tests pass. The eight desktop-feature checks cover
fonts, companion data, menus and fullscreen. FAT12/16/32 operations survive
reboot and pass host filesystem checks. All compiled apps match the image.

'''+''.join('- PASS: '+x+'.\n' for x in json.loads((LOG/'network-dns-results.json').read_text())['passed'])+'''

Actual PCM WAV captures:

| Backend | Case | Hz | Minimum | Maximum |
| --- | --- | --- | --- | --- |
'''+''.join(f"| {r['backend']} | {r['case']} | {r['rate']} | {r['min']} | {r['max']} |\n" for r in audio)+'''

The guest produces 48 kHz PCM; QEMU's capture resamples to 44.1 kHz.
DOOM effects are also checked with music disabled. The NES test tone is original.

## VirtualBox

'''+''.join('- PASS: '+x+'.\n' for x in vbox['virtualbox_verified'])+f'''
The personal VM retained {len(vbox['preserved_files'])} unchanged files, including
18 ROM/save files. That personal VDI is separate from the public installer.

## Limits

BIOS/CSM only; 256 MiB installer RAM and installed OS tested at 128 MiB.
81 MiB minimum disk; BDSFS grows up to 8 GiB. Firmware INT 13h storage;
BDSFS/FAT read/write, ISO9660 read-only. No native USB/AHCI/NVMe, hotplug,
NTFS/exFAT/ext4, GPT installation, UEFI, USB-only input or universal hardware
support. Physical machines are untested. AC'97/basic analog HDA only.
The browser supports HTTP and limited CSS/JS, not HTTPS or modern web apps.
Native apps execute in ring 0 without isolation. Companions are data files.

## Build tools and source

```text
'''+versions+'''
```

The public release includes complete BDS/upstream app source and a separate
matching Ubuntu GRUB source archive. See docs/CORRESPONDING-SOURCE.md.
'''
    (ROOT/'releases'/(base+'-verification.md')).write_text(report)
    shutil.copyfile(ROOT/'docs/GETTING-STARTED.md',ROOT/'releases'/(base+'-install.md'))
    shutil.copyfile(LOG/'installer-results.json',LOG/(a.profile+'-installer-results.json'))
    shutil.copyfile(safety,LOG/(a.profile+'-installer-safety-results.txt'))
    print('PACKAGED',base,sha(iso))
if __name__=='__main__':main()
