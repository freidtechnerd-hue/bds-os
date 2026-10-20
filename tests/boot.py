import pathlib, subprocess, time, shutil, sys, threading
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from disk import Disk
TEST=ROOT/'build/boot-test.img'
shutil.copyfile(ROOT/'build/seed.img',TEST)
d=Disk(TEST)
bad=bytearray((ROOT/'build/apps/hello.run').read_bytes());bad[6]=1
d.put('/BAD.RUN',bad);bad[6]=2;bad[-1]^=1;d.put('/BADSUM.RUN',bad);d.save()
(ROOT/'build/logs').mkdir(exist_ok=True)
def boot(name,args,inputs,expected):
    log=ROOT/f'build/logs/{name}.log'
    p=subprocess.Popen(['qemu-system-i386','-m','128','-display','none','-serial','stdio','-monitor','none','-no-reboot']+args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    chunks=[]
    def collect():
        while True:
            data=p.stdout.read1(65536)
            if not data:return
            chunks.append(data)
    reader=threading.Thread(target=collect,daemon=True);reader.start()
    def wait_for(markers,timeout=90,start=0):
        end=time.monotonic()+timeout
        while time.monotonic()<end:
            out=b''.join(chunks).decode(errors='replace')
            if all(marker in out[start:] for marker in markers):return
            if p.poll() is not None:break
            time.sleep(.1)
        raise AssertionError((name,markers,out))
    try:
        if name=='legacy':time.sleep(6)
        else:wait_for(['DESKTOP ready'])
        for data in inputs:
            start=len(b''.join(chunks).decode(errors='replace'))
            p.stdin.write(data.encode());p.stdin.flush();time.sleep(max(1.5,len(data)*.07))
            # The loader drains input on app exit. Wait for the real return
            # before sending the next command, even on a busy host.
            if name!='legacy' and data.startswith('RUN ') and 'BAD' not in data:wait_for(['RUN returned:'],start=start)
        wait_for(expected);p.terminate();p.wait(timeout=5);reader.join(timeout=5);out=b''.join(chunks).decode(errors='replace')
    finally:
        if p.poll() is None:p.kill()
    log.write_text(out)
    for marker in expected: assert marker in out,(name,marker,out)
    print(f'PASS {name}: {len(expected)} assertions; {log}')
boot('legacy',['-drive',f'file={ROOT}/build/legacy.img,format=raw,if=floppy'],['2','ECHO stale\r','ECHO\r','VER\r','DIR\r','TYPE README.TXT\r','RUN HELLO.RUN\r','NONSENSE\r'],['8086 edition','BDS:\\> ECHO\r\n\r\nBDS','README  .TXT','real BIOS boot','Hello from native 8086','Program returned','Unknown command'])
if '--legacy-only' in sys.argv:sys.exit(0)
boot('grub',['-cdrom',str(ROOT/'build/bds-os.iso'),'-drive',f'file={TEST},format=raw,if=ide,index=0'],['VER\r','RUN BAD.RUN\r','RUN BADSUM.RUN\r','RUN HELLO.RUN\r','RUN CALC.RUN 21 * 2\r','MKDIR /TEST\r','WRITE /TEST/ONE.TXT "persistent data"\r','COPY /TEST/ONE.TXT /TEST/TWO.TXT\r','MOVE /TEST/TWO.TXT /TEST/THREE.TXT\r','TYPE /TEST/THREE.TXT\r','CD /TEST\r','PWD\r','CD ..\r','CHECK\r'],['GRUB i386 edition','PASS: Multiboot2','checkpoint ready','BDSFS mounted','invalid or incompatible header','checksum failed','Hello from native i386','Result: 42','Directory created','File written and flushed','Entry moved','persistent data','/TEST','PASS: filesystem integrity'])
boot('persist',['-cdrom',str(ROOT/'build/bds-os.iso'),'-drive',f'file={TEST},format=raw,if=ide,index=0'],['TYPE /TEST/THREE.TXT\r','RMDIR /TEST\r','DEL /TEST/THREE.TXT\r','DEL /TEST/ONE.TXT\r','RMDIR /TEST\r','TYPE MISSING\r','CHECK\r'],['persistent data','Operation failed','Entry removed','PASS: filesystem integrity'])
d=Disk(TEST);d.check();assert d.find('/TEST')==-1
