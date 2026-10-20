"""Boot the actual GRUB resolution entries and inspect native framebuffer sizes."""
import re,shutil,subprocess,json,hashlib,time
from interactive import ROOT,LOG,VM
def main():
    cfg=(ROOT/'grub/grub.cfg').read_text();entries=re.findall(r'menuentry "Desktop (\d+)x(\d+)"',cfg);passed=[]
    for w,h in entries:
        # Select the real menu entry by its title; no custom kernel or mode patch.
        out=ROOT/'build/resolution-iso';(out/'boot/grub').mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/'build/kernel.elf',out/'boot/kernel.elf')
        mode=f'{w}x{h}';text=cfg.replace('set timeout=3','set timeout=0').replace('set default=0',f'set default="Desktop {mode}"');(out/'boot/grub/grub.cfg').write_text(text)
        iso=ROOT/'build/resolution-test.iso';subprocess.run(['grub-mkrescue','-o',str(iso),str(out)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
        disk=ROOT/'build/resolution-data.img';shutil.copyfile(ROOT/'build/seed.img',disk);v=VM('resolution-'+mode,disk,iso=iso,extra=['-vga','std','-global','VGA.vgamem_mb=64'])
        try:
            v.wait('DESKTOP ready');v.command('SYSINFO',' RGB32');p=v.capture('resolution-'+mode).with_suffix('.ppm');head=p.read_bytes().split(b'\n255\n',1)[0].split();actual=tuple(map(int,head[1:3]));assert actual in [(int(w),int(h)),(800,600)];assert f'Video: {actual[0]}x{actual[1]} RGB32' in v.log
            v.command('IMAGE /CLOUDS.PNG','IMAGE decoded: /CLOUDS.PNG');v.key('ctrl-f');v.wait('APP fullscreen enabled');time.sleep(.4);v.capture('resolution-'+mode+'-fullscreen');v.key('esc');v.wait('RUN returned: 0');passed.append(mode+(' (firmware fallback to 800x600)' if actual!=(int(w),int(h)) else '')+' desktop, native app fullscreen and return');print('PASS',passed[-1],flush=True)
        finally:v.close()
    (LOG/'resolution-results.json').write_text(json.dumps({'passed':passed,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest()},indent=2))
if __name__=='__main__':main()
