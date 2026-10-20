"""Native media acceptance with original generated picture/tone fixtures."""
import hashlib,json,math,os,pathlib,shutil,struct,subprocess,sys,time,wave
from interactive import ROOT,LOG,VM,pixel
sys.path.insert(0,str(ROOT/'tools'))
from disk import Disk

def fixtures(video=True):
    out=ROOT/'build/media-fixtures';out.mkdir(exist_ok=True)
    samples=b''.join(struct.pack('<hh',int(math.sin(i*2*math.pi*440/44100)*9000),int(math.sin(i*2*math.pi*440/44100)*9000)) for i in range(44100*20))
    with wave.open(str(out/'TONE.WAV'),'wb') as f:f.setparams((2,2,44100,0,'NONE','not compressed'));f.writeframes(samples)
    (out/'DISC.BIN').write_bytes(samples);(out/'DISC.CUE').write_text('FILE "DISC.BIN" BINARY\n TRACK 01 AUDIO\n  INDEX 01 00:00:00\n TRACK 02 AUDIO\n  INDEX 01 00:10:00\n')
    (out/'BAD.WAV').write_bytes(b'RIFF\xff\xff\xff\xffWAVEfmt ')
    if video:
        ffmpeg=os.environ.get('FFMPEG','ffmpeg')
        for name,codec in [('MP2','mp2'),('AC3','ac3'),('LPCM','pcm_dvd')]:
            subprocess.run([ffmpeg,'-hide_banner','-loglevel','error','-y','-f','lavfi','-i','color=c=red:size=720x480:rate=25','-f','lavfi','-i','sine=frequency=440:sample_rate=48000','-t','8','-aspect','16:9','-c:v','mpeg2video','-pix_fmt','yuv420p','-g','12','-b:v','1800k','-c:a',codec,'-ac','2','-ar','48000','-f','vob',str(out/(name+'.VOB'))],check=True)
    return out

def image(out,video=True):
    disk=ROOT/'build/media-test.img';shutil.copyfile(ROOT/'build/seed.img',disk);d=Disk(disk)
    for name in ['TONE.WAV','DISC.BIN','DISC.CUE','BAD.WAV']+(['MP2.VOB','AC3.VOB','LPCM.VOB'] if video else []):d.put('/MEDIA/'+name,(out/name).read_bytes())
    d.save();d.check();return disk

def device(name):return ['-audiodev',f'wav,id=aud,path={LOG/(name+".wav")}','-device','AC97,audiodev=aud']
def exit_player(v):
    v.pump();start=len(v.log);v.key('esc');v.wait('RUN returned: 0',start=start)

def main():
    basic='--basic' in sys.argv;out=fixtures(not basic);disk=image(out,not basic);passed=[]
    v=VM('media-wave',disk,extra=device('media-wave'))
    try:
        v.wait('DESKTOP ready');v.command('PLAY /MEDIA/TONE.WAV','PLAYER started');time.sleep(1);v.capture('media-wave')
        v.key('spc');v.wait('PLAYER: Paused');time.sleep(.25);start=(LOG/'media-wave.wav').stat().st_size;time.sleep(1);end=(LOG/'media-wave.wav').stat().st_size
        v.key('spc');v.wait('PLAYER: Playing');time.sleep(1);v.key('right');v.wait('PLAYER: Seek complete.');v.key('ctrl-f');v.wait('APP fullscreen enabled');v.capture('media-wave-fullscreen');v.key('esc');v.wait('RUN returned: 0')
        data=(LOG/'media-wave.wav').read_bytes();quiet=struct.unpack('<'+'h'*((end-start)//2),data[start:end]);assert quiet and max(map(abs,quiet))<5,'Pause continued playing PCM'
        passed.append('PCM WAV playback, silent pause, resume, seek, fullscreen and exit')
        v.command('PLAY /MEDIA/DISC.CUE','Track 1 | CD-DA');time.sleep(1);v.key('n');v.wait('Track 2 | CD-DA');v.capture('media-cue-track2');time.sleep(1);v.pump();start=len(v.log);v.key('p');v.wait('Track 1 | CD-DA',start=start);exit_player(v)
        passed.append('BIN/CUE audio-CD track selection and previous/next controls')
        v.command('PLAY /MEDIA/BAD.WAV','Unsupported or damaged media');v.key('c');v.wait('No readable audio CD.',30);exit_player(v);v.command('CHECK','PASS: filesystem integrity')
        passed.append('Malformed WAV and a data-only optical drive fail visibly; filesystem remains intact')
    finally:v.close()
    with wave.open(str(LOG/'media-wave.wav')) as f:values=struct.unpack('<'+'h'*(f.getnframes()*f.getnchannels()),f.readframes(f.getnframes()))
    assert min(values)<-3000 and max(values)>3000
    if not basic:
        for codec in ['MP2','AC3','LPCM']:
            name='media-'+codec.lower();v=VM(name,disk,extra=device(name))
            try:
                v.wait('DESKTOP ready');v.command('PLAY /MEDIA/'+codec+'.VOB','PLAYER started');v.wait('PLAYER video frame 30',180);v.capture(name)
                # Solid red source must survive actual MPEG decoding and YUV conversion.
                p=pixel(LOG/(name+'.ppm'),350,250);assert p[0]>200 and p[1]<40 and p[2]<40,p
                time.sleep(1);v.key('esc');v.wait('RUN returned: 0')
            finally:v.close()
            with wave.open(str(LOG/(name+'.wav'))) as f:
                rate=f.getframerate();raw=f.readframes(f.getnframes());values=struct.unpack('<'+'h'*(len(raw)//2),raw)
            tail=values[-rate*2:];assert min(tail)<-500 and max(tail)>500,(codec,min(tail),max(tail))
            passed.append('MPEG-2 widescreen picture decoded with '+codec+' PCM audio output')
    result={'passed':passed,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest(),'player_sha256':hashlib.sha256((ROOT/'build/apps/player.run').read_bytes()).hexdigest()}
    (LOG/'media-results.json').write_text(json.dumps(result,indent=2))
    for p in passed:print('PASS',p,flush=True)
if __name__=='__main__':main()
