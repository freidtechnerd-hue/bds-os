"""Actual DVD menu VM, automatic first-play, buttons and return to menu."""
import sys,shutil,time,json,pathlib,hashlib
from interactive import VM,ROOT,LOG
from media import device,exit_player
from display_settings import click
sys.path.insert(0,str(ROOT/'tools'));from disk import Disk

def main():
 iso=pathlib.Path(sys.argv[1]);disk=ROOT/'build/dvd-navigation.img';shutil.copyfile(ROOT/'build/seed.img',disk)
 d=Disk(disk);d.put('/PLAYER.RUN',(ROOT/'build/apps/player.run').read_bytes());d.save()
 v=VM('dvd-navigation',disk,extra=['-vga','std']+device('dvd-navigation')+['-drive',f'file={iso},format=raw,if=ide,index=3,media=cdrom,readonly=on'])
 passed=[]
 try:
  v.wait('DESKTOP ready');v.command('PLAY','DVD navigation started; autoplay',120)
  v.wait('DVD menu: arrows',180);v.wait('PLAYER video frame 30',240);time.sleep(3);v.capture('dvd-menu-initial')
  v.key('down');v.wait('DVD menu button 2 selected');time.sleep(.4);v.capture('dvd-menu-selected');v.key('up');v.wait('DVD menu button 1 selected');v.pump();at=len(v.log);v.key('ret');v.wait('DVD title 2 chapter 1',120,start=at);v.wait('PLAYER video frame 30',180,start=at)
  time.sleep(25);v.capture('dvd-menu-title');v.pump();at=len(v.log);v.key('m');v.wait('DVD menu: arrows',120,start=at);time.sleep(2);v.capture('dvd-menu-return');v.pump();at=len(v.log);click(v,400,392);v.wait('DVD title 2 chapter 1',180,start=at);v.capture('dvd-menu-mouse-play');exit_player(v)
  passed.append('Original Big Buck Bunny DVD: autoplay, authored menu, keyboard and mouse buttons, title playback and return')
 finally:
  v.close();(LOG/'dvd-navigation-results.json').write_text(json.dumps({'passed':passed,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest(),'player_sha256':hashlib.sha256((ROOT/'build/apps/player.run').read_bytes()).hexdigest()},indent=2))
if __name__=='__main__':main()
