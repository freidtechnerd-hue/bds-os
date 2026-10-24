"""Modern desktop and browser features exercised with real guest keyboard input."""
import hashlib,json,shutil,struct,subprocess,sys,time,zlib
from interactive import ROOT,LOG,VM,pixel
sys.path.insert(0,str(ROOT/'tools'));from disk import Disk
def key(v,k):v.key(k);time.sleep(.3)
def read(disk,path):
    d=Disk(disk);e=d.entries[d.find(path)];return bytes(d.data[e[4]*512:e[4]*512+e[3]])
def main():
    out=ROOT/'build/modern-probe';out.mkdir(exist_ok=True)
    for cmd in [['gcc','-m32','-O2','-ffreestanding','-fno-pie','-fno-stack-protector','-Iinclude','-c','tests/modern_probe.c','-o',out/'app.o'],['ld','-m','elf_i386','-T','apps/link.ld','-o',out/'app.elf',out/'app.o'],['objcopy','-O','binary',out/'app.elf',out/'app.bin'],['python3','tools/runpack.py',out/'app.elf',out/'app.bin',out/'probe.run']]:subprocess.run(list(map(str,cmd)),cwd=ROOT,check=True)
    disk=ROOT/'build/modern-desktop-test.img';shutil.copyfile(ROOT/'build/seed.img',disk);d=Disk(disk);d.put('/PROBE.RUN',(out/'probe.run').read_bytes())
    def chunk(t,b):return struct.pack('>I',len(b))+t+b+struct.pack('>I',zlib.crc32(t+b))
    png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',160,80,8,2,0,0,0))+chunk(b'IDAT',zlib.compress((b'\0'+b'\xff\0\0'*80+b'\0\xff\0'*80)*80))+chunk(b'IEND',b'')
    d.put('/TEST.PNG',png);d.put('/MODERN.HTM',b'<html><body><p>Inline image acceptance</p><img src="/TEST.PNG"><a href="/SECOND.HTM">Next page</a></body></html>');d.put('/SECOND.HTM',b'<html><body><p>Second history page</p></body></html>');d.save();passed=[]
    v=VM('modern-desktop',disk)
    try:
        v.wait('DESKTOP ready');v.capture('modern-desktop');v.command('RUN /PROBE.RUN','MODERN PROBE PASS');v.wait('RUN returned: 0')
        passed.append('ABI 7 clipboard bounds and persistence, architecture/heap metrics and monotonic sleep in guest')
        v.command('RUN /PROBE.RUN blit','MODERN BLIT ready');p=v.capture('modern-oversized-blit').with_suffix('.ppm')
        for x,y in [(0,0),(200,180),(799,599)]:assert pixel(p,x,y)==bytes([(x+112)&255,(y+84)&255,0x5a])
        key(v,'esc');v.wait('RUN returned: 0',start=v.log.rfind('MODERN BLIT ready'))
        passed.append('Legacy 1024x768 blit retains centered 800x600 crop without rejecting oversized frames')
        v.command('EDIT /COPY.TXT','EDITOR ready');key(v,'ctrl-v');v.wait('EDITOR pasted');key(v,'f2');v.wait('EDITOR saved');key(v,'esc');v.wait('RUN returned: 0');assert read(disk,'/COPY.TXT')==b'shared clipboard'
        v.command('EDIT /COPY.TXT','EDITOR ready');key(v,'ctrl-a');key(v,'ctrl-c');key(v,'esc');v.wait('RUN returned: 0')
        v.command('EDIT /PASTE.TXT','EDITOR ready');key(v,'ctrl-v');key(v,'f2');v.wait('EDITOR saved');key(v,'esc');v.wait('RUN returned: 0');assert read(disk,'/PASTE.TXT')==b'shared clipboard'
        passed.append('Editor Ctrl+A/C/V shares text between separate app launches and saves exact bytes')
        v.command('WEB /MODERN.HTM','BROWSER inline PNG decoded');v.wait('BROWSER page rendered: /MODERN.HTM');time.sleep(.5);p=v.capture('browser-inline-png').with_suffix('.ppm');rgb=p.read_bytes().split(b'\n255\n',1)[1];assert rgb.count(b'\xff\0\0')>1000 and rgb.count(b'\0\xff\0')>1000
        key(v,'tab');key(v,'ret');v.wait('BROWSER page rendered: /SECOND.HTM');at=len(v.log);key(v,'bracket_left');v.wait('BROWSER page rendered: /MODERN.HTM',start=at);at=len(v.log);key(v,'bracket_right');v.wait('BROWSER page rendered: /SECOND.HTM',start=at);key(v,'ctrl-c');key(v,'esc');v.wait('RUN returned: 0')
        v.command('EDIT /ADDRESS.TXT','EDITOR ready');key(v,'ctrl-v');key(v,'f2');v.wait('EDITOR saved');key(v,'esc');v.wait('RUN returned: 0');assert read(disk,'/ADDRESS.TXT')==b'/SECOND.HTM'
        passed.append('Actual inline PNG pixels, back/forward history and browser-to-editor clipboard')
        key(v,'f11');v.wait('WINDOW: maximize toggled');v.capture('modern-maximized');key(v,'f12');v.wait('WINDOW: minimized');v.capture('modern-minimized');key(v,'f1')
        key(v,'f6');key(v,'tab');key(v,'tab');key(v,'s');v.wait('SETTINGS: preferences saved');assert read(disk,'/SETTINGS.CFG')[8]==4;v.capture('modern-smooth-font-settings')
        passed.append('F11 maximize/F12 minimize and persistent antialiased font selection')
        key(v,'esc');key(v,'f1');v.command('RUN /SYSINFO.RUN','SYSTEM MONITOR ready');time.sleep(.5);v.capture('modern-system-monitor');key(v,'esc');v.wait('RUN returned: 0');v.command('CHECK','PASS: filesystem integrity')
    finally:v.close()
    v=VM('modern-preferences-cold',disk)
    try:v.wait('DESKTOP ready');assert 'SETTINGS: preferences restored' in v.log;v.capture('modern-preferences-cold');passed.append('Saved smooth font preferences restore on cold boot')
    finally:v.close()
    (LOG/'modern-desktop-results.json').write_text(json.dumps({'passed':passed,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest()},indent=2));print(json.dumps(passed,indent=2),flush=True)
if __name__=='__main__':main()
