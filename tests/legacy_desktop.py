"""The separate BIOS/FAT12 8086 edition, assembled with NASM cpu 8086."""
import hashlib,json,time
from interactive import ROOT,LOG,VM
def main():
    v=VM('legacy-desktop',None,iso=None,memory=16,extra=['-drive',f'file={ROOT}/build/legacy.img,format=raw,if=floppy']);passed=[]
    try:
        v.wait('8086 edition v0.2');v.s.sendall(b'1');v.wait('8086 DESKTOP');v.capture('legacy-desktop')
        v.s.sendall(b'2');v.wait('README  .TXT');v.s.sendall(b' ');v.wait('Select an app by number')
        v.s.sendall(b'5');v.wait('8086 Settings');at=len(v.log);v.s.sendall(b'3');v.wait('8086 Settings',start=at);v.capture('legacy-light-settings')
        v.s.sendall(b't');time.sleep(.5);v.s.sendall(b's');v.wait('Sound setting toggled');v.s.sendall(b'\x1b');time.sleep(.3);v.s.sendall(b'\x1b');v.wait('BDS:\\> ')
        v.command('RUN HELLO.RUN','Hello from native 8086');v.wait('Program returned');v.command('BEEP','BDS:\\> ');v.command('HELP','DESKTOP SETTINGS BEEP')
        passed=['Text desktop opens Files, Settings and returns to Terminal','Three BIOS text color schemes and PC speaker toggle/test respond','Native 8086 RUN loader and existing FAT12 reading still work']
    finally:v.close()
    (LOG/'legacy-desktop-results.json').write_text(json.dumps({'passed':passed,'kernel_bytes':(ROOT/'build/kernel16.bin').stat().st_size,'floppy_sha256':hashlib.sha256((ROOT/'build/legacy.img').read_bytes()).hexdigest()},indent=2));print(json.dumps(passed,indent=2),flush=True)
if __name__=='__main__':main()
