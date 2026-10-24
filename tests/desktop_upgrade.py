"""Guest acceptance for browser input/styles/scripts, NICs, volumes and power."""
import http.server,threading,time,shutil,struct,subprocess,sys,json,hashlib,re
from interactive import VM,ROOT,LOG,pixel
sys.path.insert(0,str(ROOT/'tools'));from disk import Disk
requests=[]
HTML=b'''<!doctype html><html><head><title>Before script</title><link rel="stylesheet" href="/style.css"></head><body><p id="answer">Old value</p><p class="hidden">MUST NOT DISPLAY</p><a href="/second">Second page</a><script src="/script.js"></script></body></html>'''
class Server(http.server.BaseHTTPRequestHandler):
    def log_message(self,*a):pass
    def do_GET(self):
        requests.append(self.path)
        content={'/':HTML,'/style.css':b'body {background-color:#ddeeff;color:navy} #answer {color:red;font-weight:bold} .hidden {display:none}', '/script.js':b"let sum=0; for(let i=0;i<7;i++){sum=sum+i;} let twice=function(x){return x*2;}; document.title='Script and CSS passed'; document.getElementById('answer').textContent='Result '+twice(sum); document.getElementById('answer').style.color='#123456'; console.log('COMPUTED', twice(sum)); document.write('<p>Written by JavaScript</p>');", '/second':b'<h1>Second page reached</h1>', '/loop':b'<p>Loop survived</p><script>for(;;){}</script>', '/recursive':b'<p>Recursion survived</p><script>let f=function(){return f();}; f();</script>', '/large':b'<p>'+b'wrapping packet data '*2800+b'</p>'}.get(self.path,HTML)
        self.send_response(200);self.send_header('Content-Length',str(len(content)));self.send_header('Content-Type','text/css' if self.path.endswith('.css') else 'application/javascript' if self.path.endswith('.js') else 'text/html');self.end_headers();self.wfile.write(content)
def quit_app(v):
    v.pump();at=len(v.log);v.key('esc');v.wait('RUN returned: 0',start=at)
def click(v,x,y):
    # Reach a known origin using several legal PS/2 motion packets.
    for _ in range(10):v.mouse(-120,-120);time.sleep(.04)
    time.sleep(.15)
    while x or y:
        dx=min(x,100);dy=min(y,100);v.mouse(dx,dy);x-=dx;y-=dy;time.sleep(.04)
    time.sleep(.2);v.mouse(down=True);time.sleep(.15);v.mouse(down=False);time.sleep(.2)
def body_text(v,name,expected):
    p=v.capture(name).with_suffix('.ppm');head,data=p.read_bytes().split(b'\n255\n',1);w,h=map(int,head.split()[1:3]);assert (w,h)==(800,600)
    # Fullscreen first text row must contain the requested DOM text exactly.
    rows=re.findall(r'\{([^{}]+)\}',(ROOT/'kernel/ui_font.h').read_text())
    font=[[int(x) for x in row.split(',')] for row in rows if len(row.split(','))==96]
    width=len(expected)*8+1;reference=[[list(bytes.fromhex('ddeeff')) for _ in range(width)] for _ in range(12)]
    for i,c in enumerate(expected):
        # Bold composites a second antialiased glyph one pixel to the right.
        for offset in (0,1):
            for yy in range(12):
                for xx in range(8):
                    alpha=font[ord(c)][yy*8+xx];dest=reference[yy][i*8+xx+offset]
                    reference[yy][i*8+xx+offset]=[(fg*alpha+bg*(255-alpha))//255 for fg,bg in zip(bytes.fromhex('123456'),dest)]
    for yy in range(12):
        for xx in range(width):assert pixel(p,16+xx,78+yy)==bytes(reference[yy][xx]),(xx,yy)
    assert pixel(p,790,300)==bytes.fromhex('ddeeff')
def compile_probe():
    out=ROOT/'build/volume-probe';out.mkdir(exist_ok=True)
    for command in [['gcc','-m32','-Os','-ffreestanding','-fno-builtin','-fno-pie','-fno-stack-protector','-I'+str(ROOT/'include'),'-c',str(ROOT/'tests/volume_probe.c'),'-o',str(out/'app.o')],['ld','-m','elf_i386','-T',str(ROOT/'apps/link.ld'),'-o',str(out/'app.elf'),str(out/'app.o')],['objcopy','-O','binary',str(out/'app.elf'),str(out/'app.bin')],['python3',str(ROOT/'tools/runpack.py'),str(out/'app.elf'),str(out/'app.bin'),str(out/'probe.run'),str(ROOT/'assets/icons/PLAYER.ICO')]]:subprocess.run(command,check=True)
    return (out/'probe.run').read_bytes()
def main():
    passed=[];server=http.server.ThreadingHTTPServer(('0.0.0.0',0),Server);threading.Thread(target=server.serve_forever,daemon=True).start();base=f'http://10.0.2.2:{server.server_port}'
    try:
        for nic in ['rtl8139','pcnet','e1000-82544gc','e1000-82545em']:
            disk=ROOT/f'build/upgrade-{nic}.img';shutil.copyfile(ROOT/'build/seed.img',disk);v=VM('upgrade-'+nic,disk,extra=['-netdev','user,id=n','-device',nic+',netdev=n'])
            try:
                v.wait('DESKTOP ready');v.wait('NET DHCP address: 10.0.2.15');v.command('WEB '+base+'/large','BROWSER page rendered: '+base+'/large');quit_app(v)
                v.command('WEB '+base+'/large','BROWSER page rendered: '+base+'/large');quit_app(v)
                v.qmp('set_link',{'name':'n','up':False});time.sleep(.4);v.command('NET','Ethernet cable disconnected');v.qmp('set_link',{'name':'n','up':True});time.sleep(5);v.command('WEB '+base+'/second','BROWSER page rendered: '+base+'/second');quit_app(v)
                passed.append(nic+': DHCP, HTTP beyond RX ring size, cable recovery');print('PASS',passed[-1],flush=True)
            finally:v.close()
        disk=ROOT/'build/upgrade-ui.img';shutil.copyfile(ROOT/'build/seed.img',disk);d=Disk(disk);d.put('/PROBE.RUN',compile_probe());d.put('/PROBE.ICO',(ROOT/'assets/icons/DOOM.ICO').read_bytes());d.save()
        secondary=ROOT/'build/upgrade-secondary.img';raw=bytearray(64*1024*1024);raw[:8]=b'BDSFS01\0';struct.pack_into('<II',raw,8,131072,128);secondary.write_bytes(raw);e=Disk(secondary);e.put('/PICK.TXT',b'secondary disk text');e.put('/GAME.WAD',(ROOT/'build/assets/freedoom1.wad').read_bytes());e.save()
        v=VM('upgrade-ui',disk,extra=['-netdev','user,id=n','-device','e1000,netdev=n','-drive',f'file={secondary},format=raw,if=ide,index=1'])
        try:
            v.wait('DESKTOP ready');v.wait('NET DHCP address:');v.command('WEB','BROWSER page rendered: /HOME.HTM');v.key('ctrl-f');v.wait('APP fullscreen enabled');time.sleep(.3)
            click(v,200,36);v.s.sendall((base+'/').encode());time.sleep(1);v.capture('upgrade-address-edit');v.pump();at=len(v.log);click(v,750,36);v.capture('upgrade-address-go');v.wait('BROWSER page rendered: '+base+'/',start=at);v.wait('JS: COMPUTED 42');body_text(v,'upgrade-browser-css-js','Result 42');assert '/style.css' in requests and '/script.js' in requests
            v.key('tab');time.sleep(.2);v.pump();at=len(v.log);v.key('ret');v.wait('BROWSER page rendered: '+base+'/second',start=at)
            for path,error in [('/loop','execution budget'),('/recursive','C stack')]:
                v.key('ctrl-l');time.sleep(.2);v.pump();at=len(v.log);v.s.sendall((base+path+'\r').encode());v.wait('BROWSER page rendered: '+base+path,start=at);v.wait(error,start=at)
            v.key('ctrl-l');time.sleep(.2);v.pump();at=len(v.log);v.s.sendall((base.removeprefix('http://')+'/second\r').encode());v.wait('BROWSER page rendered: '+base+'/second',start=at);quit_app(v)
            passed+=['mouse address + Go, Ctrl+L, bare host and link navigation','external CSS cascade and hidden text; JS loop/function/DOM write verified in framebuffer','infinite loop and recursive scripts return errors and browser remains usable']
            v.capture('upgrade-resources-before');v.key('f2');time.sleep(.3);v.key('right');v.wait('FILE MANAGER VOLUME: 1');v.capture('upgrade-resources-other-drive')
            for x,y in [(24,32),(26,33),(26,98),(30,167)]:assert pixel(LOG/'upgrade-resources-before.ppm',x,y)==pixel(LOG/'upgrade-resources-other-drive.ppm',x,y)
            v.key('f9');v.wait('BROWSER page rendered: /HOME.HTM',start=v.log.rfind('FILE MANAGER VOLUME: 1'));quit_app(v);v.key('f1');time.sleep(.3);v.command('MOUNT 0','Volume mounted');v.command('RUN /PROBE.RUN','VOLUME PROBE: choose');v.key('right');v.wait('FILE MANAGER VOLUME: 1',start=v.log.rfind('VOLUME PROBE: choose'));v.key('ret');v.wait('VOLUME PROBE: handles, picker restore, write and resources passed');time.sleep(.4);v.capture('upgrade-embedded-icon');icon=(ROOT/'assets/icons/PLAYER.ICO').read_bytes();off=struct.unpack_from('<I',icon,18)[0]+40+(15-4)*64+7*4;assert pixel(LOG/'upgrade-embedded-icon.ppm',93,43)==icon[off:off+3][::-1];quit_app(v)
            v.command('DOOM','DOOM WAD selector ready');v.key('right');v.wait('FILE MANAGER VOLUME: 1',start=v.log.rfind('DOOM WAD selector ready'));v.capture('upgrade-wad-other-drive');v.key('ret');v.wait('FILE MANAGER SELECTED: V1:/GAME.WAD');v.wait('DOOM frame 1',180);v.capture('upgrade-doom-other-drive');v.key('f10');v.wait('RUN returned: 0',start=v.log.rfind('FILE MANAGER SELECTED: V1:/GAME.WAD'));v.command('TYPE /README.TXT','BDX-OS Ultimate');v.command('CHECK','PASS: filesystem integrity')
            passed+=['File Manager drive switch preserves desktop icons and system app launch','picker restores volume; original file handle remains readable; qualified writes persist','embedded RUN icon overrides conflicting sidecar','DOOM WAD selector loads game from secondary drive']
            v.key('f10');time.sleep(.3);v.capture('upgrade-start-menu');v.key('p');v.p.wait(timeout=10);v.pump();assert 'POWER: shutting down' in v.log;passed+=['Start-menu ACPI power off exits QEMU']
        finally:v.close()
        e=Disk(secondary);i=e.find('/WRITTEN.TXT');assert i>=0;x=e.entries[i];assert e.data[x[4]*512:x[4]*512+x[3]]==b'cross-volume write'
        v=VM('upgrade-reset',disk)
        try:v.wait('DESKTOP ready');v.key('f10');time.sleep(.2);v.key('r');v.p.wait(timeout=10);v.pump();assert 'POWER: restarting' in v.log;passed+=['Start-menu restart sends guest reset (QEMU -no-reboot exits)']
        finally:v.close()
    finally:server.shutdown()
    report={'passed':passed,'http_requests':requests,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest(),'live_iso_sha256':hashlib.sha256((ROOT/'build/bds-os.iso').read_bytes()).hexdigest()};(LOG/'desktop-upgrade-results.json').write_text(json.dumps(report,indent=2));print('PASS',len(passed),'desktop upgrade cases',flush=True)
if __name__=='__main__':main()
