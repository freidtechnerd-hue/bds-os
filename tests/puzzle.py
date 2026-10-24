"""Native i386/x64 2048 input, merge, undo and cold-boot save persistence."""
import hashlib,json,shutil,struct,sys,time
from interactive import ROOT,LOG,VM
sys.path.insert(0,str(ROOT/'tools'));from disk import Disk
def state(path):
    d=Disk(path);e=d.entries[d.find('/PUZZLE.SAV')];return bytes(d.data[e[4]*512:e[4]*512+e[3]])
def main():
    passed=[]
    for bits in [32,64]:
        root=ROOT/('build' if bits==32 else 'build/x64');disk=ROOT/f'build/puzzle-{bits}-test.img';shutil.copyfile(root/'seed.img',disk)
        original=struct.pack('<III16B',0x31584442,0,1234,1,1,2,*([0]*13));d=Disk(disk);d.put('/PUZZLE.SAV',original);d.save()
        def vm(name):return VM(name,disk,iso=root/('bds-os.iso' if bits==32 else 'bds-os-x64.iso'),memory=128 if bits==32 else 256,emulator='qemu-system-i386' if bits==32 else 'qemu-system-x86_64')
        v=vm(f'puzzle-{bits}')
        try:
            v.wait('DESKTOP ready',90);v.command('RUN /PUZZLE.RUN','2048 ready');v.key('left');v.wait('2048 move');v.key('f2');v.wait('Game saved');saved=state(disk);assert struct.unpack_from('<I',saved,4)[0]==4 and saved[12:14]==b'\2\2';v.capture(f'puzzle-{bits}-merged')
            v.key('u');time.sleep(.3);at=len(v.log);v.key('f2');v.wait('Game saved',start=at);assert state(disk)==original;v.key('esc');v.wait('RUN returned: 0');v.command('CHECK','PASS: filesystem integrity')
        finally:v.close()
        v=vm(f'puzzle-{bits}-cold')
        try:v.wait('DESKTOP ready',90);v.command('RUN /PUZZLE.RUN','2048 restored');v.capture(f'puzzle-{bits}-cold');v.key('esc');v.wait('RUN returned: 0');assert state(disk)==original
        finally:v.close()
        passed.append(f'{bits}-bit native 2048: real arrow input merges once, save bytes verified, undo restores exact state and cold boot resumes')
    (LOG/'puzzle-results.json').write_text(json.dumps({'passed':passed},indent=2));print(json.dumps(passed,indent=2),flush=True)
if __name__=='__main__':main()
