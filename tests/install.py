"""Destructive installer acceptance uses ONLY disposable image files.
Boot install media as optical and USB, detach it, then cold-boot target.
"""
import hashlib,json,pathlib,re,shutil,struct,subprocess,sys,time
from interactive import VM,ROOT,LOG
sys.path.insert(0,str(ROOT/'tools'))
from disk import Disk
ISO=ROOT/'build/bds-os-installer.iso'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def blank(name,size=128):
    p=ROOT/f'build/{name}.img'
    with p.open('wb') as f:f.truncate(size*1048576)
    with p.open('r+b') as f:f.write(b'KEEP MY EXISTING DATA')
    return p
def install(v):
    v.wait('Target disk number:',60)
    disks=re.findall(r'DISK (\d+) \| ([^\n]+)',v.log)
    target=next(int(n) for n,desc in disks if 'writable' in desc and 'boot media' not in desc)
    v.s.sendall(f'{target}\r'.encode());v.wait('Confirmation:');v.capture(v.name+'-confirm')
    # Firmware write + read-back verification can be slow on a host-mounted
    # NTFS image. Keep all byte checks, but allow a realistic installation time.
    v.s.sendall(f'ERASE {target}\r'.encode());v.wait('INSTALL COMPLETE:',600);v.capture(v.name+'-complete');v.key('esc');v.wait('BDS command line ready.');return target
def check_fat(v):
    v.command('MOUNT 0','Volume mounted');v.command('TYPE /BOOT/GRUB/GRUB.CFG','Graphical Desktop')
    v.command('MKDIR /TEST','Directory created');v.command('WRITE /TEST/ONE.TXT "persistent FAT data"','File written')
    v.command('COPY /TEST/ONE.TXT /TEST/TWO.TXT','File copied');v.command('MOVE /TEST/TWO.TXT /TEST/THREE.TXT','Entry moved')
    v.command('TYPE /TEST/THREE.TXT','persistent FAT data');v.command('MOUNT 1','Volume mounted')
def cold(v):
    v.wait('DESKTOP ready',45);v.command('TYPE /INSTALLED.TXT','disk-only persistence');v.command('RUN /HELLO.RUN','Hello from native i386')
    v.command('NES','NES library');start=len(v.log);v.key('esc');v.wait('RUN returned: 0',start=start);check_fat(v);v.capture(v.name+'-desktop')
def main():
    results=[];disk=blank('install-target');before=digest(disk)
    for action in ['cancel','wrong-confirmation']:
        v=VM('installer-'+action,disk,iso=ISO,memory=256,extra=['-boot','d'])
        try:
            v.wait('Target disk number:',60)
            if action=='cancel':v.key('esc');v.wait('Cancelled.')
            else:v.s.sendall(b'0\r');v.wait('Confirmation:');v.s.sendall(b'NO\r');v.wait('No disks changed.')
        finally:v.close()
        assert digest(disk)==before;results.append(action+' leaves target byte-for-byte unchanged')
    v=VM('installer-cd',disk,iso=ISO,memory=256,extra=['-boot','d'])
    try:
        install(v);v.command('WRITE /INSTALLED.TXT "disk-only persistence"','File written');v.command('RUN /HELLO.RUN','Hello from native i386')
        v.command('MOUNT 2','Volume mounted');v.command('TYPE /README.TXT','BDX-OS BIOS installer');v.command('WRITE /BLOCKED.TXT nope','Operation failed');v.command('MOUNT 1','Volume mounted')
    finally:v.close()
    results.append('optical installation, full read-back, optical filesystem and write rejection')
    v=VM('installed-ide',disk,iso=None)
    try:cold(v)
    finally:v.close()
    v=VM('installed-ide-persist',disk,iso=None)
    try:
        v.wait('DESKTOP ready');v.command('MOUNT 0','Volume mounted');v.command('TYPE /TEST/THREE.TXT','persistent FAT data');v.command('DEL /TEST/THREE.TXT','Entry removed');v.command('DEL /TEST/ONE.TXT','Entry removed');v.command('RMDIR /TEST','Entry removed')
    finally:v.close()
    results.append('cold disk-only IDE boot, apps/NES, FAT nested create/copy/move/delete and persistence')
    # Another valid BDSFS must not replace the boot disk's data volume.
    decoy=ROOT/'build/install-decoy.img';shutil.copyfile(ROOT/'build/seed.img',decoy)
    other=Disk(decoy);other.put('/INSTALLED.TXT',b'wrong disk');other.save();decoy_before=digest(decoy)
    v=VM('installed-multiple-disks',decoy,iso=None,extra=['-drive',f'file={disk},format=raw,if=none,id=system','-device','ide-hd,drive=system,bus=ide.0,unit=1,bootindex=1'])
    try:
        v.wait('DESKTOP ready');assert 'boot media' in v.log
        assert v.log.count('BDSFS read/write')==2
        v.command('TYPE /INSTALLED.TXT','disk-only persistence');v.command('WRITE /BOOTDISK.TXT "correct boot volume"','File written')
        v.capture('installed-multiple-disks')
    finally:v.close()
    assert digest(decoy)==decoy_before
    results.append('multiple BDS disks: boot volume selected and other disk unchanged')
    # Same installed image, attached to AHCI SATA rather than IDE.
    v=VM('installed-ahci',None,iso=None,extra=['-device','ich9-ahci,id=ahci','-drive',f'file={disk},format=raw,if=none,id=sata','-device','ide-hd,drive=sata,bus=ahci.0'])
    try:v.wait('DESKTOP ready');v.command('TYPE /INSTALLED.TXT','disk-only persistence');v.command('WRITE /SATA.TXT native','File written');v.command('RUN /HELLO.RUN','Hello from native i386');v.capture('installed-ahci')
    finally:v.close()
    results.append('AHCI SATA boot and read/write via firmware')
    # Boot the hybrid ISO as USB mass storage, install to a different disk.
    usbtarget=blank('install-usb-target');usbbefore=digest(ISO)
    usb=['-device','usb-ehci,id=ehci','-drive',f'file={ISO},format=raw,if=none,id=media,readonly=on','-device','usb-storage,drive=media,bus=ehci.0,bootindex=1']
    v=VM('installer-usb',usbtarget,iso=None,memory=256,extra=usb)
    try:install(v);v.command('WRITE /INSTALLED.TXT "disk-only persistence"','File written')
    finally:v.close()
    assert digest(ISO)==usbbefore
    v=VM('installed-usb-target',usbtarget,iso=None)
    try:v.wait('DESKTOP ready');v.command('TYPE /INSTALLED.TXT','disk-only persistence');v.command('RUN /HELLO.RUN','Hello from native i386')
    finally:v.close()
    results.append('hybrid ISO USB boot, separate target installation and cold boot without USB')
    # Also boot the installed OS directly from writable USB mass storage.
    v=VM('installed-usb-drive',None,iso=None,extra=['-device','usb-ehci,id=ehci','-drive',f'file={disk},format=raw,if=none,id=stick','-device','usb-storage,drive=stick,bus=ehci.0,bootindex=1'])
    try:v.wait('DESKTOP ready');v.command('TYPE /INSTALLED.TXT','disk-only persistence');v.command('WRITE /USB.TXT "USB persistence"','File written');v.command('RUN /HELLO.RUN','Hello from native i386');v.capture('installed-usb-drive')
    finally:v.close()
    results.append('installed USB drive standalone boot and read/write')
    (LOG/'installer-results.json').write_text(json.dumps({'passed':results,'installer_sha256':digest(ISO)},indent=2))
    for r in results:print('PASS',r,flush=True)
if __name__=='__main__':main()
