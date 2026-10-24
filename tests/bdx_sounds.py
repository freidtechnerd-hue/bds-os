"""Verify supplied cues reach actual AC97/HDA PCM output, not just log messages."""
import array,json,shutil,sys,time,wave
from interactive import ROOT,LOG,VM
sys.path.insert(0,str(ROOT/'tools'));from disk import Disk
def main():
    passed=[]
    for backend in ['AC97','HDA']:
        disk=ROOT/f'build/bdx-sounds-{backend}.img';shutil.copyfile(ROOT/'build/seed.img',disk);d=Disk(disk);d.put('/DOOM.RUN',b'invalid');d.save()
        wav=LOG/f'bdx-sounds-{backend}.wav';devices=['-device','AC97,audiodev=aud'] if backend=='AC97' else ['-device','intel-hda','-device','hda-output,audiodev=aud']
        v=VM('bdx-sounds-'+backend,disk,extra=['-audiodev',f'wav,id=aud,path={wav},out.frequency=48000']+devices)
        try:
            v.wait('DESKTOP ready');v.wait('SFX startup chime queued');v.key('f6');v.wait('SFX window');time.sleep(1)
            v.key('tab');time.sleep(.3);v.key('2');v.wait('SFX notification');time.sleep(1)
            v.key('f5');time.sleep(1);v.key('d');v.wait('SFX error');time.sleep(2)
        finally:v.close()
        with wave.open(str(wav)) as f:assert f.getframerate()==48000 and f.getnchannels()==2;stereo=array.array('h',f.readframes(f.getnframes()))
        left=stereo[::2]
        for cue in ['START','CLICK','DING','ERROR']:
            with wave.open(str(ROOT/'assets/sounds'/f'{cue}.WAV')) as f:mono=array.array('h',f.readframes(f.getnframes()))
            # A varying non-silent run distinguishes each actual supplied recording.
            offset=max(range(len(mono)-64),key=lambda i:abs(mono[i]));pattern=mono[offset:offset+64]
            assert abs(pattern[0])>1000
            matched=False
            for i in range(len(left)-64):
                gain=left[i]/pattern[0]
                if .1<=gain<=1.1 and all(abs(left[i+j]-pattern[j]*gain)<4 for j in (8,16,24,32,48,63)) and all(abs(left[i+j]-pattern[j]*gain)<4 for j in range(64)):
                    matched=True;break
            assert matched,(backend,cue,'expected supplied PCM waveform absent')
        passed.append(backend+': supplied waveform samples (allowing hardware mixer gain) from TADA, click, DING and CHORD captured at the emulated audio device')
    (LOG/'bdx-sounds-results.json').write_text(json.dumps({'passed':passed},indent=2));print(json.dumps(passed,indent=2),flush=True)
if __name__=='__main__':main()
