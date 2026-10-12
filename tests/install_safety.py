"""Negative installer cases: GPT target, boot media, low RAM, bad payload."""
import hashlib,re,shutil,struct,subprocess
from interactive import VM,ROOT,LOG
from install import ISO,blank,digest
def unchanged(v,disk,before,marker):
    try:v.wait(marker,90)
    finally:v.close()
    assert digest(disk)==before
def main():
    disk=blank('install-safety-target');before=digest(disk)
    v=VM('installer-low-ram',disk,iso=ISO,memory=128,extra=['-boot','d'])
    unchanged(v,disk,before,'FATAL: installer requires 256 MiB RAM')
    print('PASS low RAM fails visibly without changing target',flush=True)
    with disk.open('r+b') as f:
        mbr=bytearray(512);struct.pack_into('<B3sB3sII',mbr,446,0,b'\0'*3,0xee,b'\0'*3,1,128*2048-1);mbr[510:]=b'\x55\xaa';f.write(mbr);f.write(b'EFI PART')
    before=digest(disk);v=VM('installer-gpt',disk,iso=ISO,memory=256,extra=['-boot','d'])
    try:v.wait('Target disk number:',90);v.s.sendall(b'0\r');v.wait('GPT target detected.');v.capture('installer-gpt-refusal')
    finally:v.close()
    assert digest(disk)==before;print('PASS existing GPT disk refused unchanged',flush=True)
    usb=['-device','usb-ehci,id=ehci','-drive',f'file={ISO},format=raw,if=none,id=media,readonly=on','-device','usb-storage,drive=media,bus=ehci.0,bootindex=1']
    v=VM('installer-own-usb',disk,iso=None,memory=256,extra=usb);isobefore=digest(ISO)
    try:
        v.wait('Target disk number:',90);boot=next(int(n) for n,d in re.findall(r'DISK (\d+) \| ([^\n]+)',v.log) if 'boot media' in d)
        v.s.sendall(f'{boot}\r'.encode());v.wait('No disks changed.')
    finally:v.close()
    assert digest(disk)==before and digest(ISO)==isobefore;print('PASS installer boot USB cannot be selected',flush=True)
    out=ROOT/'build/installer-safety';(out/'boot/grub').mkdir(parents=True,exist_ok=True)
    shutil.copyfile(ROOT/'build/kernel.elf',out/'boot/kernel.elf')
    payload=bytearray((ROOT/'build/installer/system.img').read_bytes());payload[1000000]^=1;(out/'bad.img').write_bytes(payload)
    (out/'boot/grub/grub.cfg').write_text('set timeout=0\nmenuentry "Corrupt installer test" { multiboot2 /boot/kernel.elf installer; module2 /bad.img system-image; boot; }\n')
    badiso=ROOT/'build/installer-badsum.iso';subprocess.run(['grub-mkrescue','-o',str(badiso),str(out)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    v=VM('installer-badsum',disk,iso=badiso,memory=256,extra=['-boot','d'])
    unchanged(v,disk,before,'Installer payload checksum failed.')
    print('PASS corrupted installation payload refused unchanged',flush=True)
    (LOG/'installer-safety-results.txt').write_text('PASS low RAM, existing GPT, installer boot USB, corrupted payload: no target bytes changed.\n')
if __name__=='__main__':main()
