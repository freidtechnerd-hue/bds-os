"""Real AC'97/HDA DMA playback captured through QEMU's WAV audio backend."""
import pathlib,shutil,struct,time,wave,json
from interactive import VM,ROOT,LOG
import sys
sys.path.insert(0,str(ROOT/'tools'))
from disk import Disk

def tone_cartridge():
    # Original mapper-0 cartridge: a constant-volume 440 Hz APU pulse.
    code=bytearray([0x78,0xd8,0xa2,0xff,0x9a])
    for address,value in [(0x4017,0x40),(0x4015,1),(0x4000,0xbf),(0x4001,0),(0x4002,253),(0x4003,8)]:
        code.extend([0xa9,value,0x8d,address&255,address>>8])
    loop=0x8000+len(code);code.extend([0x4c,loop&255,loop>>8])
    prg=bytearray([0xea]*16384);prg[:len(code)]=code
    for offset in [16378,16380,16382]:struct.pack_into('<H',prg,offset,0x8000)
    return b'NES\x1a'+bytes([1,0,0,0])+bytes(8)+prg
def capture(name,command,marker,seconds=3,close='esc',backend='AC97'):
    disk=ROOT/f'build/audio-{name}.img';shutil.copyfile(ROOT/'build/seed.img',disk)
    if name.startswith('nes'):
        d=Disk(disk);d.put('/NES/TONE.NES',tone_cartridge());d.save()
    wav=LOG/f'audio-{name}.wav';wav.unlink(missing_ok=True)
    devices=['-device','AC97,audiodev=aud'] if backend=='AC97' else ['-device','intel-hda','-device','hda-output,audiodev=aud']
    v=VM('audio-'+name,disk,extra=['-audiodev',f'wav,id=aud,path={wav}']+devices)
    try:
        v.wait('DESKTOP ready');assert ("AC'97 ready" if backend=='AC97' else 'HDA analog output ready') in v.log
        v.command(command,marker);v.wait('APP window frame ready');v.capture('audio-'+name);time.sleep(seconds)
        if name=='doom':
            start=len(v.log);v.key('ctrl-f');v.wait('APP fullscreen enabled',start=start);time.sleep(.3);v.capture('doom-fullscreen');start=len(v.log);v.key('ctrl-f');v.wait('APP windowed enabled',start=start);v.key('up',900);v.key('f',500);time.sleep(1)
        elif name=='doom-sfx':
            assert 'DOOM native PCM sound effects ready' in v.log
            assert 'MIDI software synthesis started' not in v.log
            v.key('f',1300);time.sleep(2)
        start=len(v.log);v.key(close);v.wait('RUN returned: 0',start=start)
    finally:v.close()
    with wave.open(str(wav)) as f:
        assert f.getsampwidth()==2;data=f.readframes(f.getnframes());rate=f.getframerate()
    values=struct.unpack('<'+'h'*(len(data)//2),data);assert min(values)<-100 and max(values)>100,(name,min(values),max(values))
    print('PASS',backend,name,'captured',rate,'Hz PCM range',min(values),max(values),flush=True)
    return {'case':name,'backend':backend,'rate':rate,'min':min(values),'max':max(values)}
if __name__=='__main__':
    results=[capture('mp3','RUN /PLAYER.RUN /MUSIC/DEMO.MP3','MP3 decoding started'),capture('midi','RUN /PLAYER.RUN /MUSIC/DEMO.MID','MIDI software synthesis started'),capture('doom','RUN /DOOM.RUN -iwad /FREEDOOM1.WAD -warp 1 1','DOOM playable level active',3,'f10'),capture('doom-sfx','RUN /DOOM.RUN -iwad /FREEDOOM1.WAD -nomusic -warp 1 1','DOOM playable level active',1,'f10'),capture('nes-tone','NES /NES/TONE.NES','NES frame 120'),capture('hda-mp3','PLAY /MUSIC/DEMO.MP3','MP3 decoding started',3,'esc','HDA'),capture('hda-midi','PLAY /MUSIC/DEMO.MID','MIDI software synthesis started',3,'esc','HDA'),capture('nes-hda','NES /NES/TONE.NES','NES frame 120',3,'esc','HDA')]
    (LOG/'audio-results.json').write_text(json.dumps(results,indent=2))
