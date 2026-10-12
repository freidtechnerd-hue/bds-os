"""Validate FAT formats against standard host tools after guest writes."""
import json,pathlib,shutil,subprocess
from interactive import VM,ROOT,LOG
def run(*args):return subprocess.run(list(map(str,args)),check=True,capture_output=True,text=True).stdout
def main():
    results=[];base=ROOT/'build/storage-base.img';shutil.copyfile(ROOT/'build/seed.img',base)
    source=ROOT/'build/fat-long-name-source.txt';source.write_text('original long filename data')
    for bits,size in [(12,4),(16,16),(32,64)]:
        disk=ROOT/f'build/storage-fat{bits}.img'
        with disk.open('wb') as f:f.truncate(size*1048576)
        run('mkfs.fat','-F',bits,disk);run('mcopy','-i',disk,source,'::/Long File Name.txt')
        v=VM(f'storage-fat{bits}',base,extra=['-device','usb-ehci,id=ehci','-drive',f'file={disk},format=raw,if=none,id=stick','-device','usb-storage,drive=stick,bus=ehci.0'])
        try:
            v.wait('DESKTOP ready');v.command('MOUNT 1','Volume mounted');v.command('TYPE /LONGFI~1.TXT','original long filename data')
            v.command('MOVE /LONGFI~1.TXT /RENAMED.TXT','Entry moved');v.command('MKDIR /TEST','Directory created')
            v.command('WRITE /TEST/ONE.TXT "FAT persistent data"','File written');v.command('COPY /TEST/ONE.TXT /TEST/TWO.TXT','File copied')
            v.command('MOVE /TEST /MOVED','Entry moved');v.command('TYPE /MOVED/TWO.TXT','FAT persistent data')
            v.command('DEL /MOVED/TWO.TXT','Entry removed');v.command('MOUNT 0','Volume mounted');v.command('RUN HELLO.RUN','Hello from native i386')
        finally:v.close()
        assert 'original long filename data' in run('mtype','-i',disk,'::/RENAMED.TXT')
        assert 'FAT persistent data' in run('mtype','-i',disk,'::/MOVED/ONE.TXT')
        listing=run('mdir','-i',disk,'::/');assert 'Long File Name' not in listing
        check=run('fsck.fat','-n',disk);(LOG/f'fat{bits}-fsck.log').write_text(check)
        results.append(f'FAT{bits}: USB discovery, long-name alias rename, directory move, persistent read/write, host fsck')
        print('PASS',results[-1],flush=True)
    (LOG/'storage-results.json').write_text(json.dumps(results,indent=2))
if __name__=='__main__':main()
