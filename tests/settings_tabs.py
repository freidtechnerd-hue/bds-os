"""Real guest acceptance for tabs, persisted preferences and undistorted apps."""
import hashlib,json,re,shutil,subprocess,time,sys
from interactive import ROOT,LOG,VM,pixel
from display_settings import press,click
from media import device
sys.path.insert(0,str(ROOT/'tools'))
from disk import Disk

def config(disk):
 d=Disk(disk);e=d.entries[d.find('/SETTINGS.CFG')];return bytes(d.data[e[4]*512:e[4]*512+e[3]])

def main():
 out=ROOT/'build/display-probe';out.mkdir(exist_ok=True)
 for cmd in [['gcc','-m32','-Os','-ffreestanding','-fno-builtin','-fno-pie','-fno-stack-protector','-I'+str(ROOT/'include'),'-c',str(ROOT/'tests/display_probe.c'),'-o',str(out/'app.o')],['ld','-m','elf_i386','-T',str(ROOT/'apps/link.ld'),'-o',str(out/'app.elf'),str(out/'app.o')],['objcopy','-O','binary',str(out/'app.elf'),str(out/'app.bin')],['python3',str(ROOT/'tools/runpack.py'),str(out/'app.elf'),str(out/'app.bin'),str(out/'probe.run')]]:subprocess.run(cmd,check=True)
 disk=ROOT/'build/settings-tabs.img';shutil.copyfile(ROOT/'build/seed.img',disk);d=Disk(disk);d.put('/PROBE.RUN',(out/'probe.run').read_bytes());d.save();passed=[]
 v=VM('settings-tabs',disk,extra=device('settings-tabs'))
 try:
  v.wait('DESKTOP ready');v.command('MOUNT 1','Volume mounted');press(v,'f6');v.capture('settings-display-tab')
  press(v,'tab');v.wait('SETTINGS: tab Colors');press(v,'2');assert config(disk)[4]==1;v.capture('settings-colors-tab')
  press(v,'tab');v.wait('SETTINGS: tab Fonts');press(v,'h');assert config(disk)[8]==2;v.capture('settings-fonts-tab')
  # File picker cancellation retains the selected font; it also restores the browsed volume.
  press(v,'o');v.wait('FILE MANAGER OPEN: .PSF');press(v,'esc');assert config(disk)[8]==2
  press(v,'o');press(v,'right') # optical drive -> system BDSFS volume
  d=Disk(disk);folders=[e[0].rstrip(b'\0') for e in d.entries if e[2]==2 and e[1]==0]
  for _ in range(folders.index(b'FONTS')):press(v,'down')
  press(v,'ret');fonts=[e[0].rstrip(b'\0') for e in d.entries if e[2]==1 and e[1]==d.find('/FONTS')+1 and e[0].rstrip(b'\0').endswith(b'.PSF')]
  for _ in range(fonts.index(b'BOLD.PSF')):press(v,'down')
  press(v,'ret');assert config(disk)[8]==3
  d=Disk(disk);e=d.entries[d.find('/FONTS/USER.PSF')];assert bytes(d.data[e[4]*512:e[4]*512+e[3]])==(ROOT/'assets/fonts/BOLD.PSF').read_bytes()
  passed.append('Custom PSF selected through File Manager is copied to the system font directory and saved')
  press(v,'h')
  press(v,'tab');v.wait('SETTINGS: tab Sounds');press(v,'minus');press(v,'m');press(v,'c');assert config(disk)[4:9]==bytes([1,90,1,0,2]);v.capture('settings-sounds-tab')
  passed.append('All four Settings tabs; theme, bold font, mute, master volume and chimes saved to system volume while browsing CD')
  press(v,'m');press(v,'c');at=len(v.log);press(v,'t');v.wait('SFX startup chime queued',start=at)
  passed.append('Font picker cancellation and startup-sound preview work')
 finally:v.close()
 v=VM('settings-tabs-cold',disk,extra=device('settings-tabs-cold'))
 try:
  v.wait('DESKTOP ready');assert 'SETTINGS: preferences restored' in v.log;time.sleep(.5);v.capture('settings-persisted');assert pixel(LOG/'settings-persisted.ppm',790,540)==bytes.fromhex('2b2921')
  press(v,'f6');press(v,'tab');press(v,'tab');press(v,'d');press(v,'tab');v.capture('settings-sounds-restored');assert config(disk)[4:9]==bytes([1,90,0,1,1]);passed.append('Cold boot restores saved theme, font and sound preferences')
 finally:v.close()
 # Use a real saved mode; firmware must actually expose this widescreen size.
 d=Disk(disk);d.put('/DISPLAY.CFG',b'1280 720\n');d.save();v=VM('display-aspect',disk)
 try:
  v.wait('DISPLAY: restored 1280x720');v.wait('DESKTOP ready');v.command('RUN /PROBE.RUN','DISPLAY PROBE ready');press(v,'ctrl-f');v.wait('APP fullscreen enabled');press(v,'v');v.wait('VIEWPORT 960x720');p=v.capture('display-aspect').with_suffix('.ppm')
  assert pixel(p,80,300)==bytes(3) and pixel(p,1200,300)==bytes(3)
  data=p.read_bytes().split(b'\n255\n',1)[1]
  def px(x,y):return data[(y*1280+x)*3:(y*1280+x)*3+3]
  xs=[x for x in range(1280) if px(x,340)==b'\xff\0\0'];ys=[y for y in range(720) if px(600,y)==b'\xff\0\0'];assert len(xs)==len(ys)==120
  # Click the content at a known logical coordinate, then click a border.
  click(v,640,360);v.wait('POINTER 400,300');v.pump();at=len(v.log);click(v,50,360);v.pump();assert 'POINTER' not in v.log[at:]
  passed.append('1280x720 fullscreen has a square 120x120 probe, 960x720 content, black borders and correctly mapped pointer; borders reject clicks')
  press(v,'ctrl-f');v.wait('APP windowed enabled');press(v,'esc');v.wait('RUN returned: 0');v.command('CHECK','PASS: filesystem integrity')
 finally:v.close()
 (LOG/'settings-tabs-results.json').write_text(json.dumps({'passed':passed,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest()},indent=2))
 for p in passed:print('PASS',p,flush=True)
if __name__=='__main__':main()
