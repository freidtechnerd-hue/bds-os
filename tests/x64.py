"""Boot the actual long-mode kernel, native apps and durable ATA filesystem."""
import hashlib,json,shutil,sys,time,os,re
from interactive import ROOT,LOG,VM
sys.path.insert(0,str(ROOT/'tools'));from disk import Disk
def vm(name,disk=None):return VM(name,disk,iso=ROOT/'build/x64/bds-os-x64.iso',memory=256,extra=['-cpu','qemu64'],emulator='qemu-system-x86_64')
def main():
    passed=[];v=vm('x64-live')
    try:
        v.wait('DESKTOP ready');assert 'PASS: 64-bit C' in v.log;assert 'read-only live module' in v.log
        registers=v.qmp('human-monitor-command',{'command-line':'info registers'})
        assert int(re.search(r'EFER=([0-9a-fA-F]+)',registers)[1],16)&0x500==0x500,registers
        v.command('RUN /HELLO.RUN','RUN returned: 0');v.command('RUN /CALC.RUN 21 * 2','Result: 42');v.wait('RUN returned: 0')
        v.command('FONT /FONTS/DEJAVU.TTF','FONT: TrueType rasterized');v.command('FONT DEFAULT','Built-in font restored')
        v.command('WRITE /RO.TXT no','Operation failed');v.command('CHECK','PASS: filesystem integrity');v.capture('x64-live')
        passed.append('Actual long mode, shared desktop, native 64-bit HELLO/CALC, live filesystem rejects writes')
    finally:v.close()
    disk=ROOT/f'build/x64/acceptance-{os.getpid()}.img';shutil.copyfile(ROOT/'build/x64/seed.img',disk)
    d=Disk(disk);d.put('/WRONG.RUN',(ROOT/'build/apps/hello.run').read_bytes());d.save()
    v=vm('x64-ata',disk)
    try:
        v.wait('DESKTOP ready');assert 'native ATA PIO writable' in v.log
        v.command('MKDIR /TEST','Directory created');v.command('WRITE /TEST/ONE.TXT durable-x64','File written and flushed')
        v.command('COPY /TEST/ONE.TXT /TEST/TWO.TXT','File copied');v.command('MOVE /TEST/TWO.TXT /TEST/THREE.TXT','Entry moved')
        v.command('RUN /WRONG.RUN','invalid or incompatible header');v.command('CHECK','PASS: filesystem integrity')
        passed.append('Native ATA PIO directory/write/copy/move and explicit rejection of i386 RUN binary')
    finally:v.close()
    v=vm('x64-cold',disk)
    try:
        v.wait('DESKTOP ready');v.command('TYPE /TEST/THREE.TXT','durable-x64');v.command('CHECK','PASS: filesystem integrity')
        v.command('GAME snake','RUN loaded:');time.sleep(2);v.capture('x64-native-game');v.key('esc');v.wait('RUN returned: 0')
        passed.append('Cold boot persistent data and native 64-bit graphical game exits safely')
    finally:v.close()
    Disk(disk).check();report={'passed':passed,'kernel_sha256':hashlib.sha256((ROOT/'build/x64/kernel.elf').read_bytes()).hexdigest(),'iso_sha256':hashlib.sha256((ROOT/'build/x64/bds-os-x64.iso').read_bytes()).hexdigest()}
    (LOG/'x64-results.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2),flush=True)
if __name__=='__main__':main()
