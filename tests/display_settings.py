"""Exercise live VBE switching through real Settings input and cold boots."""
import hashlib,json,shutil,struct,subprocess,time,sys,re
from interactive import ROOT,LOG,VM
sys.path.insert(0,str(ROOT/'tools'))
from disk import Disk

def saved(path):
    d=Disk(path);i=d.find('/DISPLAY.CFG')
    if i<0:return None
    e=d.entries[i];return bytes(d.data[e[4]*512:e[4]*512+e[3]])

def size(v,expected,name):
    time.sleep(.3);p=v.capture(name)
    assert struct.unpack('>II',p.read_bytes()[16:24])==expected

def press(v,key):v.key(key);time.sleep(.25)

def click(v,x,y):
    for _ in range(20):v.mouse(-120,-120);time.sleep(.04)
    while x or y:
        dx=min(x,100);dy=min(y,100);v.mouse(dx,dy);x-=dx;y-=dy;time.sleep(.04)
    v.mouse(down=True);time.sleep(.15);v.mouse(down=False);time.sleep(.2)

def main():
    disk=ROOT/'build/display-settings-test.img';shutil.copyfile(ROOT/'build/seed.img',disk);passed=[]
    v=VM('display-settings',disk)
    try:
        v.wait('DESKTOP ready');assert 'DISPLAY: supported 640x480 800x600 1024x768' in v.log
        v.command('MOUNT 1','Volume mounted')
        press(v,'f6');size(v,(800,600),'display-settings-initial')
        press(v,'right');press(v,'ret');v.wait('DISPLAY: preview started');size(v,(1024,768),'display-settings-preview')
        press(v,'esc');v.wait('DISPLAY: preview reverted');size(v,(800,600),'display-settings-reverted')
        assert saved(disk) is None;passed.append('Preview and Escape restore framebuffer without saving')
        start=len(v.log);press(v,'right');press(v,'ret');v.wait('DISPLAY: preview reverted',30,start);size(v,(800,600),'display-settings-timeout')
        assert saved(disk) is None;passed.append('Unconfirmed preview automatically reverts after 15 seconds')
        press(v,'right');press(v,'ret');press(v,'ret');v.wait('DISPLAY: saved 1024x768');assert saved(disk)==b'1024 768\n'
        passed.append('Keep persists on system volume even while browsing read-only CD')
        modes=re.search(r'DISPLAY: supported ([^\n]+)',v.log).group(1).split()
        for _ in range(modes.index('1920x1200')-modes.index('1024x768')):press(v,'right')
        press(v,'ret')
        size(v,(1920,1200),'display-settings-largest');press(v,'f9')
        assert 'APP windowed launch' not in v.log
        press(v,'esc');size(v,(1024,768),'display-settings-largest-revert')
        assert saved(disk)==b'1024 768\n';passed.append('1920x1200 live preview, modal app blocking and restore')
        press(v,'f1');v.command('MOUNT 0','Volume mounted');v.command('CHECK','PASS: filesystem integrity');v.command('IMAGE /CLOUDS.PNG','IMAGE decoded: /CLOUDS.PNG')
        press(v,'ctrl-f');v.wait('APP fullscreen enabled');size(v,(1024,768),'display-settings-app-fullscreen')
        press(v,'esc');v.wait('RUN returned: 0');v.command('SYSINFO','Video: 1024x768 RGB32');passed.append('Disk BIOS calls and native fullscreen still work after video BIOS calls')
    finally:v.close()
    v=VM('display-settings-cold',disk)
    try:
        v.wait('DESKTOP ready');assert 'DISPLAY: restored 1024x768' in v.log;size(v,(1024,768),'display-settings-cold');passed.append('Normal cold boot restores saved size')
        press(v,'f6');press(v,'left');press(v,'left');press(v,'ret');press(v,'ret');v.wait('DISPLAY: saved 640x480');size(v,(640,480),'display-settings-small')
        assert saved(disk)==b'640 480\n';passed.append('640x480 remains usable and can be confirmed')
        click(v,400,172);click(v,100,316);v.wait('DISPLAY: switched 800x600');click(v,100,316)
        v.wait('DISPLAY: saved 800x600');size(v,(800,600),'display-settings-mouse');passed.append('Mouse resolution selection, Apply and Keep work after shrinking windows')
    finally:v.close()
    d=Disk(disk);d.put('/DISPLAY.CFG',b'1024 768\n');d.save()
    out=ROOT/'build/display-recovery-iso';(out/'boot/grub').mkdir(parents=True,exist_ok=True)
    shutil.copyfile(ROOT/'build/kernel.elf',out/'boot/kernel.elf')
    cfg=(ROOT/'grub/grub.cfg').read_text().replace('set timeout=3','set timeout=0').replace('set default=0','set default="Desktop 800x600 (ignore saved settings)"')
    (out/'boot/grub/grub.cfg').write_text(cfg);iso=ROOT/'build/display-recovery.iso'
    subprocess.run(['grub-mkrescue','-o',str(iso),str(out)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    v=VM('display-settings-recovery',disk,iso=iso)
    try:
        v.wait('DESKTOP ready');size(v,(800,600),'display-settings-recovery');assert 'DISPLAY: restored' not in v.log;assert saved(disk)==b'1024 768\n';passed.append('Recovery GRUB entry ignores saved size without deleting it')
        press(v,'f6');press(v,'ret');press(v,'ret');v.wait('DISPLAY: saved 800x600');assert saved(disk)==b'800 600\n';passed.append('Recovery mode can replace the saved size by applying the current resolution')
    finally:v.close()
    for config in [b'9999 9999\n',b'1024 garbage\n']:
        d=Disk(disk);d.put('/DISPLAY.CFG',config);d.save();v=VM('display-settings-invalid',disk)
        try:v.wait('DESKTOP ready');size(v,(800,600),'display-settings-invalid')
        finally:v.close()
    passed.append('Unsupported and malformed saved sizes fall back to boot mode')
    (LOG/'display-settings-results.json').write_text(json.dumps({'passed':passed,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest()},indent=2))
    for p in passed:print('PASS',p,flush=True)

if __name__=='__main__':main()
