"""Real QEMU UI acceptance: framebuffer captures, PS/2 input and native apps."""
import json,os,pathlib,socket,subprocess,time,zlib,struct,shutil
ROOT=pathlib.Path(__file__).resolve().parents[1];LOG=ROOT/'build/logs';LOG.mkdir(exist_ok=True)
def png(ppm):
    data=pathlib.Path(ppm).read_bytes();head,pixels=data.split(b'\n255\n',1);parts=head.split();w,h=map(int,parts[1:3])
    def chunk(t,d):return struct.pack('>I',len(d))+t+d+struct.pack('>I',zlib.crc32(t+d))
    scan=b''.join(b'\0'+pixels[y*w*3:(y+1)*w*3] for y in range(h))
    out=pathlib.Path(ppm).with_suffix('.png');out.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(scan))+chunk(b'IEND',b''));return out
def pixel(ppm,x,y):
    head,data=pathlib.Path(ppm).read_bytes().split(b'\n255\n',1);w=int(head.split()[1]);p=(y*w+x)*3;return data[p:p+3]
class VM:
    def __init__(self,name,disk,serial=True,iso=ROOT/"build/bds-os.iso",memory=128,extra=()):
        self.name=name;self.log='';base=f'/tmp/bds-{os.getpid()}-{name}';self.qpath=base+'-qmp';self.spath=base+'-serial'
        self.p=subprocess.Popen(['qemu-system-i386','-m',str(memory)]+(['-cdrom',str(iso)] if iso else [])+(['-drive',f'file={disk},format=raw,if=ide,index=0'] if disk else [])+list(extra)+['-display','none','-qmp',f'unix:{self.qpath},server=on,wait=off','-serial',f'unix:{self.spath},server=on,wait=off' if serial else 'none','-no-reboot'],stderr=open(LOG/f'{name}-qemu.log','w'))
        for _ in range(100):
            if pathlib.Path(self.qpath).exists() and (not serial or pathlib.Path(self.spath).exists()):break
            time.sleep(.05)
        self.q=socket.socket(socket.AF_UNIX);self.q.connect(self.qpath);self.f=self.q.makefile('rwb',buffering=0);self.f.readline();self.qmp('qmp_capabilities')
        self.s=None
        if serial:
            self.s=socket.socket(socket.AF_UNIX);self.s.connect(self.spath);self.s.setblocking(False)
    def qmp(self,cmd,args=None):
        d={'execute':cmd}
        if args:d['arguments']=args
        self.f.write(json.dumps(d).encode()+b'\n')
        while True:
            r=json.loads(self.f.readline())
            if 'error' in r:raise RuntimeError(r)
            if 'return' in r:return r['return']
    def pump(self):
        if self.s is None:return
        while True:
            try:data=self.s.recv(65536)
            except BlockingIOError:break
            if not data:break
            self.log+=data.decode(errors='replace')
        (LOG/f'{self.name}.log').write_text(self.log)
    def wait(self,marker,timeout=90,start=0):
        end=time.monotonic()+timeout
        while time.monotonic()<end:
            self.pump()
            if marker in self.log[start:]:return
            if 'FATAL' in self.log:raise AssertionError(self.log)
            if 'DOOM exit to BDS-OS' in self.log[start:] and marker not in ('DOOM exit to BDS-OS','RUN returned: 0','BDS-OS Ultimate 0.2'):raise AssertionError(self.log)
            time.sleep(.1)
        raise AssertionError(f'Timeout: {marker}\n{self.log[-3000:]}')
    def command(self,s,marker,timeout=90):
        self.pump();start=len(self.log);self.s.sendall((s+'\r').encode());self.wait(marker,timeout,start)
    def key(self,k,hold=100):self.qmp('human-monitor-command',{'command-line':f'sendkey {k} {hold}'})
    def mouse(self,x=0,y=0,down=None):
        events=[]
        if x:events.append({'type':'rel','data':{'axis':'x','value':x}})
        if y:events.append({'type':'rel','data':{'axis':'y','value':y}})
        if down is not None:events.append({'type':'btn','data':{'button':'left','down':down}})
        self.qmp('input-send-event',{'events':events})
    def capture(self,name):
        ppm=LOG/f'{name}.ppm';self.qmp('screendump',{'filename':str(ppm)});out=png(ppm);print('Capture',out,flush=True);return out
    def close(self):
        self.pump();self.p.terminate();self.p.wait(timeout=10);self.q.close()
        if self.s is not None:self.s.close()
        for p in [self.qpath,self.spath]:pathlib.Path(p).unlink(missing_ok=True)
def no_serial_test():
    disk=ROOT/'build/no-serial-test.img';shutil.copyfile(ROOT/'build/seed.img',disk);v=VM('no-serial',disk,serial=False)
    try:
        ppm=LOG/'no-serial.ppm'
        def await_pixel(x,y,color,timeout=15):
            end=time.monotonic()+timeout
            while time.monotonic()<end:
                v.qmp('screendump',{'filename':str(ppm)})
                if pixel(ppm,x,y)==color:return
                if v.p.poll() is not None:raise AssertionError('No-serial VM exited')
                time.sleep(.2)
            png(ppm)
            raise AssertionError(f'No-serial framebuffer/input timed out at {x},{y}')
        # Firmware reads the BMP splash before drawing the desktop. Allow the
        # same cold-boot budget used by the serial tests, even under contention.
        await_pixel(72,56,b'\xff\xff\xff',90)
        v.key('f2');await_pixel(101,134,b'\x41\x6b\x58')
        v.capture('no-serial-file-manager')
        v.key('f6');time.sleep(.3);v.key('2');await_pixel(790,540,b'\x2b\x29\x21')
        v.capture('no-serial-settings')
        print('PASS absent COM1: desktop, mounted file listing, PS/2 input and theme change',flush=True)
    finally:v.close()
def ui_test():
    # User data may contain a saved display preference; use a reproducible fixture.
    disk=ROOT/'build/ui-test.img';shutil.copyfile(ROOT/'build/seed.img',disk);v=VM('ui',disk)
    try:
        v.wait('DESKTOP ready');time.sleep(.5);before=v.capture('desktop').read_bytes()
        v.mouse(-300,-230);time.sleep(.3);v.mouse(down=True);time.sleep(.2);v.mouse(80,50);time.sleep(.3);v.mouse(down=False);time.sleep(.2);after=v.capture('mouse-drag').read_bytes();assert before!=after
        assert pixel(LOG/'desktop.ppm',72,56)==b'\xff\xff\xff'
        assert pixel(LOG/'mouse-drag.ppm',72,56)!=b'\xff\xff\xff'
        assert pixel(LOG/'mouse-drag.ppm',152,106)==b'\xff\xff\xff'
        v.key('f2');time.sleep(.5);v.capture('file-manager');v.key('f6');time.sleep(.4);v.key('right');v.key('2');time.sleep(.4);v.capture('settings');v.key('esc');v.key('f1');time.sleep(.5)
        v.command('EDIT /UI.TXT','EDITOR ready');v.s.sendall(b'Native editor saved text');time.sleep(1);v.key('f2');v.wait('EDITOR saved');v.capture('editor');v.key('esc');v.wait('RUN returned: 0');v.command('TYPE /UI.TXT','Native editor saved text')
        for g in ['snake','pong','blocks','mines','arcade']:
            start=len(v.log);v.command('GAME '+g,'GAME started:');v.wait('GAME frames: 100',start=start);v.key('right');v.key('spc');time.sleep(.3);v.capture('game-'+g);v.key('r');time.sleep(.2);v.key('esc');v.wait('GAME exit',start=start);time.sleep(.5)
        v.command('CHECK','PASS: filesystem integrity');print('PASS desktop, native editor persistence, five game loops and exits',flush=True)
    finally:v.close()
def doom_test():
    disk=ROOT/'build/doom-test.img';shutil.copyfile(ROOT/'build/seed.img',disk);v=VM('doom',disk)
    try:
        v.wait('DESKTOP ready');v.command('RUN /DOOM.RUN -iwad /FREEDOOM1.WAD -nosound -warp 1 1','DOOM playable level active',180)
        v.wait('DOOM frame 100');v.capture('doom-level');v.key('up',900);v.wait('DOOM player moved');time.sleep(1);v.capture('doom-moved');v.key('f10');v.wait('DOOM exit to BDS-OS');v.wait('RUN returned: 0');v.command('VER','BDS-OS Ultimate 0.2');v.capture('doom-return');print('PASS DOOM native load, WAD, playable level, frames, movement, clean exit, fresh image boot',flush=True)
    finally:v.close()
if __name__=='__main__':
    import sys
    if len(sys.argv)>1 and sys.argv[1]=='doom':doom_test()
    elif len(sys.argv)>1 and sys.argv[1]=='no-serial':no_serial_test()
    else:ui_test();no_serial_test()
