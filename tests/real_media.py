"""Optional tests against caller-supplied, legally obtained DVD/CD fixtures.
No downloads occur here. CD fixture is an ISO9660 volume containing GOLDBERG.CUE/BIN.
"""
import argparse,hashlib,json,pathlib,shutil,struct,time,wave
from interactive import ROOT,LOG,VM
from media import device,exit_player
import sys
sys.path.insert(0,str(ROOT/'tools'))
from disk import Disk

def main():
    a=argparse.ArgumentParser();a.add_argument('--cd',type=pathlib.Path);a.add_argument('--dvd',type=pathlib.Path);args=a.parse_args();passed=[]
    for kind,iso in [('cd',args.cd),('dvd',args.dvd)]:
        if not iso:continue
        disk=ROOT/f'build/real-{kind}.img';shutil.copyfile(ROOT/'build/seed.img',disk)
        d=Disk(disk);d.put('/BAD.WAV',b'broken');d.save()
        name='real-'+kind
        v=VM(name,disk,extra=device(name)+['-drive',f'file={iso},format=raw,if=ide,index=3,media=cdrom,readonly=on'])
        try:
            v.wait('DESKTOP ready');v.command('DRIVES','ISO9660 read-only')
            if kind=='cd':
                v.command('PLAY V2:/GOLDBERG.CUE','Track 1 | CD-DA');time.sleep(8);v.capture('real-cd-track1');v.key('n');v.wait('Track 2 | CD-DA');time.sleep(8);v.capture('real-cd-track2')
                v.key('right');v.wait('Seek complete.');time.sleep(2);v.key('ctrl-f');v.wait('APP fullscreen enabled');v.capture('real-cd-fullscreen');exit_player(v)
                passed.append('Downloaded Open Goldberg recording: authored two-track BIN/CUE streams from optical filesystem, track switch, seek and fullscreen')
            else:
                v.command('PLAY /BAD.WAV','Unsupported or damaged media');v.key('d');v.wait('Choose a DVD title.');v.capture('real-dvd-titles');v.key('down');time.sleep(.3);v.key('ret');v.wait('PLAYER video frame 30',300);time.sleep(5);v.key('right');v.wait('Seek complete.');time.sleep(15);v.capture('real-dvd-playing');v.key('ctrl-f');v.wait('APP fullscreen enabled');time.sleep(8);v.capture('real-dvd-fullscreen');exit_player(v)
                passed.append('Original Big Buck Bunny DVD ISO: mounted VIDEO_TS, title selection, decoded movie picture, fullscreen and clean exit; DVD menus unsupported')
            v.command('CHECK','PASS: filesystem integrity')
        finally:v.close()
        with wave.open(str(LOG/(name+'.wav'))) as f:
            rate=f.getframerate();data=f.readframes(f.getnframes());pcm=struct.unpack('<'+'h'*(len(data)//2),data)
        # Ignore startup chime: inspect the final six seconds of captured playback.
        tail=pcm[-rate*12:];assert tail and min(tail)<-100 and max(tail)>100,(kind,min(tail),max(tail))
        print('PASS',passed[-1],'PCM range',min(tail),max(tail),flush=True)
    suffix='dvd' if args.dvd else 'cd'
    (LOG/f'real-media-{suffix}-results.json').write_text(json.dumps({'passed':passed,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest(),'player_sha256':hashlib.sha256((ROOT/'build/apps/player.run').read_bytes()).hexdigest()},indent=2))
if __name__=='__main__':main()
