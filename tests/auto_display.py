"""Automatic EDID resolution selection in real VBE firmware."""
import hashlib,json,shutil,struct,time,sys
from interactive import ROOT,LOG,VM
from display_settings import press
sys.path.insert(0,str(ROOT/'tools'))
from disk import Disk

def persistence():
 disk=ROOT/'build/auto-display-persistence.img';shutil.copyfile(ROOT/'build/seed.img',disk)
 d=Disk(disk);d.put('/DISPLAY.CFG',b'auto\n');d.save();passed=[]
 for name,w,h,expected in [('saved',1280,720,(1280,720)),('changed',1024,768,(1024,768)),('manual',1920,1080,(800,600))]:
  if name=='manual':
   d=Disk(disk);d.put('/DISPLAY.CFG',b'800 600\n');d.save()
  v=VM('auto-preference-'+name,disk,auto_display=True,extra=['-vga','std','-global',f'VGA.xres={w}','-global',f'VGA.yres={h}'])
  try:
   v.wait('DESKTOP ready');p=v.capture('auto-preference-'+name)
   assert struct.unpack('>II',p.read_bytes()[16:24])==expected
   passed.append(name)
  finally:v.close()
 report={'passed':['Saved Auto restores on cold boot','Saved Auto follows a changed monitor preference','Explicit saved size overrides monitor preference'],'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest()}
 (LOG/'auto-display-persistence-results.json').write_text(json.dumps(report,indent=2));print('PASS',passed,flush=True)
def main():
 passed=[]
 for w,h in [(1280,720),(1024,768),(3840,2160),(0,0)]:
  disk=ROOT/'build/auto-display.img';shutil.copyfile(ROOT/'build/seed.img',disk)
  extra=['-vga','std','-global','VGA.vgamem_mb=64','-global',f'VGA.edid={"on" if w else "off"}']
  if w:extra+=['-global',f'VGA.xres={w}','-global',f'VGA.yres={h}']
  v=VM(f'auto-{w}-{h}',disk,extra=extra,auto_display=True)
  try:
   v.wait('DESKTOP ready');time.sleep(.5);p=v.capture(f'auto-{w}-{h}');actual=struct.unpack('>II',p.read_bytes()[16:24]);print('Auto firmware:',actual,v.log[v.log.index('DISPLAY:'):v.log.index('AUDIO:')],flush=True)
   assert actual==((w,h) if w else (800,600)),actual
   press(v,'f6');press(v,'u');press(v,'ret');v.wait('DISPLAY: automatic preference saved')
   d=Disk(disk);e=d.entries[d.find('/DISPLAY.CFG')];assert bytes(d.data[e[4]*512:e[4]*512+e[3]])==b'auto\n'
   passed.append(f'EDID {w}x{h}: automatic boot and saved Auto mode' if w else 'Missing EDID retains safe boot mode')
  finally:v.close()
 (LOG/'auto-display-results.json').write_text(json.dumps({'passed':passed,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest()},indent=2))
if __name__=='__main__':main();persistence()
