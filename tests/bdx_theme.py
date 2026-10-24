"""Guest TrueType loading, graphic window controls and durable font selection."""
import hashlib,json,shutil,sys,time
from interactive import ROOT,LOG,VM
from desktop_upgrade import click
sys.path.insert(0,str(ROOT/'tools'));from disk import Disk
def read(d,path):
    disk=Disk(d);i=disk.find(path);assert i>=0;entry=disk.entries[i];return bytes(disk.data[entry[4]*512:entry[4]*512+entry[3]])
def main():
    disk=ROOT/'build/bdx-theme-test.img';shutil.copyfile(ROOT/'build/seed.img',disk);d=Disk(disk)
    d.put('/BAD.TTF',b'\0\1\0\0'+b'\xff'*96);fonts=[]
    for name in ['arial.ttf','times.ttf','comic.ttf']:
        p=ROOT/('/mnt/c/Windows/Fonts/'+name)
        if p.exists():d.put('/'+name.upper(),p.read_bytes());fonts.append('/'+name.upper())
    d.save();passed=[];v=VM('bdx-theme',disk)
    try:
        v.wait('DESKTOP ready');v.capture('bdx-desktop')
        for path in ['/FONTS/DEJAVU.TTF']+fonts:
            v.command('FONT '+path,'Custom font loaded');v.capture('bdx-font-'+path.split('/')[-1].lower())
        at=len(v.log);v.command('FONT /BAD.TTF','FONT: invalid PSF/TrueType font');assert 'FATAL' not in v.log[at:]
        passed.append('Guest FreeType renders '+', '.join(['/FONTS/DEJAVU.TTF']+fonts)+' and rejects malformed TTF')
        v.command('FONT DEFAULT','Built-in font restored')
        v.s.sendall(b'CD /FONTS\r');time.sleep(.5);v.key('f6');time.sleep(.3);v.key('tab');time.sleep(.2);v.key('tab');time.sleep(.2);v.key('t');v.wait('FILE MANAGER OPEN: .TTF');v.key('ret');v.wait('SETTINGS: preferences saved');assert read(disk,'/SETTINGS.CFG')[8]==5;assert read(disk,'/FONTS/USER.TTF')==read(disk,'/FONTS/DEJAVU.TTF');v.capture('bdx-ttf-settings')
        passed.append('Settings TrueType picker copies the font to the system volume and persists choice 5')
        v.key('esc');v.key('f1');time.sleep(.3);click(v,640,72);v.wait('WINDOW: maximize toggled');v.capture('bdx-window-maximized')
        click(v,742,23);time.sleep(.3) # restore using the maximized graphic button
        click(v,620,72);v.wait('WINDOW: minimized');v.key('f1');time.sleep(.3)
        v.command('EDIT /BDX.TXT','EDITOR ready');click(v,658,44);v.wait('APP minimized');v.capture('bdx-app-minimized');click(v,200,580);v.wait('APP restored');click(v,684,44);v.wait('APP fullscreen enabled');v.key('ctrl-f');v.wait('APP windowed enabled');click(v,708,44);v.wait('RUN returned: 0');v.command('CHECK','PASS: filesystem integrity')
        passed.append('Graphic desktop maximize/minimize and native-app minimize/restore/fullscreen/close work with mouse input')
    finally:v.close()
    v=VM('bdx-font-cold',disk)
    try:v.wait('DESKTOP ready');assert 'FONT: TrueType rasterized' in v.log and 'SETTINGS: preferences restored' in v.log;v.capture('bdx-font-cold');passed.append('TrueType font survives a cold boot')
    finally:v.close()
    (LOG/'bdx-theme-results.json').write_text(json.dumps({'passed':passed,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest()},indent=2));print(json.dumps(passed,indent=2),flush=True)
if __name__=='__main__':main()
