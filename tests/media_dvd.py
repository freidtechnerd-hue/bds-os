"""DVD title/part navigation, widescreen aspect and failure acceptance."""
import hashlib,json,shutil,struct,subprocess,time,wave
from interactive import ROOT,LOG,VM,pixel
from media import device,exit_player
import sys
sys.path.insert(0,str(ROOT/'tools'))
from disk import Disk

def main():
    out=ROOT/'build/media-dvd-iso';(out/'boot/grub').mkdir(parents=True,exist_ok=True);(out/'VIDEO_TS').mkdir(exist_ok=True)
    (out/'ABONUS').mkdir(exist_ok=True)
    for i in range(160):(out/'ABONUS'/f'FILE{i:03}.TXT').write_text('Original directory-limit fixture\n')
    shutil.copyfile(ROOT/'build/kernel.elf',out/'boot/kernel.elf')
    cfg=(ROOT/'grub/grub.cfg').read_text().replace('set timeout=3','set timeout=0').replace('set default=0','set default="Desktop 1280x720"')
    (out/'boot/grub/grub.cfg').write_text(cfg)
    for n in [1,2]:shutil.copyfile(ROOT/'build/media-fixtures/AC3.VOB',out/f'VIDEO_TS/VTS_01_{n}.VOB')
    iso=ROOT/'build/media-dvd.iso';subprocess.run(['grub-mkrescue','-o',str(iso),str(out)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    disk=ROOT/'build/media-dvd.img';shutil.copyfile(ROOT/'build/seed.img',disk);d=Disk(disk)
    d.put('/BAD.WAV',b'broken');d.put('/CRYPT.VOB',bytes.fromhex('000001e000088000002345678900'))
    # MPEG-2 PES scrambling bits, with an otherwise bounded packet.
    d.put('/CRYPT.VOB',bytes.fromhex('000001e00008b000002345678900'));d.save()
    passed=[];v=VM('media-dvd',disk,iso=iso,extra=device('media-dvd'))
    try:
        v.wait('DESKTOP ready');v.command('SYSINFO','Video: 1280x720 RGB32')
        v.command('PLAY /BAD.WAV','Unsupported or damaged media');v.key('d');v.wait('Choose a DVD title.');v.capture('media-dvd-titles');v.key('ret');v.wait('PLAYER video frame 30',180)
        v.key('ctrl-f');v.wait('APP fullscreen enabled');time.sleep(.4);p=v.capture('media-dvd-widescreen').with_suffix('.ppm')
        assert pixel(p,640,300)[0]>200
        # A 16:9 frame occupies 712x400 logical pixels on a 1280x720 screen:
        # actual picture is about 854x480? UI's aspect correction must yield 16:9.
        head,data=p.read_bytes().split(b'\n255\n',1);w,h=map(int,head.split()[1:3]);assert (w,h)==(1280,720)
        def red(x,y):
            q=(y*w+x)*3;return data[q]>200 and data[q+1]<40 and data[q+2]<40
        xs=[x for x in range(w) if red(x,300)];ys=[y for y in range(80,580) if red(640,y)]
        assert xs and ys and abs((max(xs)-min(xs)+1)/(max(ys)-min(ys)+1)-16/9)<.02
        passed.append('VIDEO_TS survives 160 bonus files; optical title picker and actual 1280x720 fullscreen 16:9 picture')
        v.key('spc');v.wait('PLAYER: Paused');time.sleep(.3);start=(LOG/'media-dvd.wav').stat().st_size;time.sleep(.8);end=(LOG/'media-dvd.wav').stat().st_size
        data=(LOG/'media-dvd.wav').read_bytes()[start:end];samples=struct.unpack('<'+'h'*(len(data)//2),data);assert samples and max(map(abs,samples))<5
        v.pump();at=len(v.log);v.key('spc');v.wait('PLAYER: Playing',start=at);v.key('right');v.wait('Seek complete.')
        v.pump();at=len(v.log);v.key('n');v.wait('PLAYER started',start=at);v.wait('PLAYER video frame 30',180,start=at);v.capture('media-dvd-part2');exit_player(v)
        passed.append('Video silent pause/resume, approximate seek and next VOB part')
        v.command('PLAY /CRYPT.VOB','Encrypted DVD stream:');exit_player(v);v.command('CHECK','PASS: filesystem integrity')
        passed.append('Scrambled DVD stream rejected visibly without harming filesystem')
    finally:v.close()
    v=VM('media-no-sse2',disk,extra=['-cpu','pentium3'])
    try:
        v.wait('DESKTOP ready');v.command('PLAY /BAD.WAV','Media Player requires an SSE2 CPU.');v.wait('RUN returned: 1')
        passed.append('Non-SSE2 CPU is rejected before executing media SSE instructions')
    finally:v.close()
    (LOG/'media-dvd-results.json').write_text(json.dumps({'passed':passed,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest()},indent=2))
    for p in passed:print('PASS',p,flush=True)
if __name__=='__main__':main()
