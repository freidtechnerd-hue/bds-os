"""Real QEMU boot animation, image/WAD UI, DMA startup audio and Ethernet/HTTP."""
import http.server,threading,time,shutil,json,struct,sys,wave,subprocess,hashlib
from interactive import VM,ROOT,LOG,pixel
sys.path.insert(0,str(ROOT/'tools'));from disk import Disk
requests=[]
class Server(http.server.BaseHTTPRequestHandler):
    def log_message(self,*args):pass
    def do_GET(self):
        requests.append(self.path)
        if self.path=='/redirect':
            self.send_response(302);self.send_header('Location','/second');self.end_headers();return
        body=(b'<html><h1>Ethernet acceptance</h1><p>HTML entity: &amp;.</p><a href="/second">Next page</a></html>' if self.path=='/' else b'<html><h1>Second page via TCP</h1><p>Link and redirect reached this page.</p></html>')
        if self.path=='/large':body=b'x'*(3*1024*1024)
        self.send_response(200);self.send_header('Content-Type','text/html')
        if self.path=='/chunk':
            self.send_header('Transfer-Encoding','chunked');self.end_headers()
            for part in [body[:27],body[27:]]:self.wfile.write(('%x\r\n'%len(part)).encode()+part+b'\r\n')
            self.wfile.write(b'0\r\n\r\n')
        else:
            self.send_header('Content-Length',str(len(body)));self.end_headers()
            try:self.wfile.write(body)
            except (BrokenPipeError,ConnectionResetError):pass

def quit_app(v):
    v.pump();start=len(v.log);v.key('esc');v.wait('RUN returned: 0',start=start)
def main():
    tested_kernel=hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest();tested_iso=hashlib.sha256((ROOT/'build/bds-os.iso').read_bytes()).hexdigest()
    results=[];disk=ROOT/'build/network-desktop-test.img';shutil.copyfile(ROOT/'build/seed.img',disk);d=Disk(disk);d.put('/BROKEN.BMP',b'BM'+bytes(70))
    out=ROOT/'build/read-probe';out.mkdir(exist_ok=True)
    subprocess.run(['gcc','-m32','-Os','-ffreestanding','-fno-pie','-fno-stack-protector','-I'+str(ROOT/'include'),'-c',str(ROOT/'tests/read_probe.c'),'-o',str(out/'probe.o')],check=True)
    subprocess.run(['ld','-m','elf_i386','-T',str(ROOT/'apps/link.ld'),'-o',str(out/'probe.elf'),str(out/'probe.o')],check=True)
    subprocess.run(['objcopy','-O','binary',str(out/'probe.elf'),str(out/'probe.bin')],check=True)
    subprocess.run(['python3',str(ROOT/'tools/runpack.py'),str(out/'probe.elf'),str(out/'probe.bin'),str(out/'probe.run')],check=True)
    d.put('/READTEST.BIN',bytes((i*37+(i>>8))&255 for i in range(98317)));d.put('/READPROB.RUN',(out/'probe.run').read_bytes());d.save()
    server=http.server.ThreadingHTTPServer(('0.0.0.0',0),Server);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();base=f'http://10.0.2.2:{server.server_port}'
    wav=LOG/'startup-ac97.wav';wav.unlink(missing_ok=True)
    v=VM('network-desktop',disk,extra=['-netdev','user,id=n','-device','e1000,netdev=n','-audiodev',f'wav,id=a,path={wav}','-device','AC97,audiodev=a'])
    try:
        v.wait('BOOT BMP splash ready');start=time.monotonic();v.capture('boot-progress-early');time.sleep(.9);v.capture('boot-progress-middle');v.wait('BOOT progress complete after 3000 ms');elapsed=time.monotonic()-start;assert 2.6<elapsed<5.5,elapsed
        assert (LOG/'boot-progress-early.ppm').read_bytes()!=(LOG/'boot-progress-middle.ppm').read_bytes();v.wait('DESKTOP ready');v.wait('NET DHCP address: 10.0.2.15');v.capture('cloud-wallpaper-desktop')
        # Smooth mode reconstructs the supplied image, preserving its aspect.
        # Compare with a floating-point bilinear reference (guest uses fixed point).
        bmp=(ROOT/'assets/WALLPAPR.BMP').read_bytes();off=struct.unpack_from('<I',bmp,10)[0];bw,bh=struct.unpack_from('<ii',bmp,18);sw,sh=bw,bh
        if 800*bh>600*bw:sh=bw*600//800
        else:sw=bh*800//600
        sx,sy=(bw-sw)//2+750*(sw-1)/799,(bh-sh)//2+500*(sh-1)/599;x,y=int(sx),int(sy);ax,ay=sx-x,sy-y
        def source(x,y):p=off+(bh-1-y)*((bw*3+3)&~3)+x*3;return bmp[p:p+3][::-1]
        expected=[int(source(x,y)[c]*(1-ax)*(1-ay)+source(x+1,y)[c]*ax*(1-ay)+source(x,y+1)[c]*(1-ax)*ay+source(x+1,y+1)[c]*ax*ay) for c in range(3)]
        actual=pixel(LOG/'cloud-wallpaper-desktop.ppm',750,500);assert all(abs(a-b)<=2 for a,b in zip(actual,expected));results+=['3-second animated boot bar','smooth supplied cloud wallpaper pixels','Intel PRO/1000 DHCP lease']
        v.command('RUN /READPROB.RUN','READ PROBE: bulk, unaligned and EOF bytes verified');results+=['guest bulk-read boundaries and EOF verified byte-for-byte']
        v.command('IMAGE /CLOUDS.PNG','IMAGE decoded: /CLOUDS.PNG');v.capture('image-viewer-png');v.key('ctrl-f');v.wait('APP fullscreen enabled');v.capture('image-viewer-fullscreen');v.key('o');v.wait('FILE MANAGER OPEN: .BMP;.PNG');v.key('esc');time.sleep(.2);quit_app(v)
        v.command('IMAGE /WALLPAPR.BMP','IMAGE decoded: /WALLPAPR.BMP');v.capture('image-viewer-bmp');quit_app(v)
        v.command('IMAGE /BROKEN.BMP','IMAGE viewer ready');v.wait('APP window frame ready',start=v.log.rfind('IMAGE viewer ready'));v.capture('image-viewer-invalid');quit_app(v);results+=['PNG/BMP viewer and fullscreen','damaged-image handling']
        v.command('WEB','BROWSER page rendered: /HOME.HTM');v.capture('browser-home');quit_app(v)
        v.command('WEB '+base+'/','BROWSER page rendered: '+base+'/');v.capture('browser-http');v.pump();at=len(v.log);v.key('tab');time.sleep(.2);v.key('ret');v.wait('BROWSER page rendered: '+base+'/second',start=at);assert '/' in requests and '/second' in requests;v.capture('browser-linked-page')
        v.pump();at=len(v.log);v.s.sendall(('l'+base+'/chunk\r').encode());v.wait('BROWSER page rendered: '+base+'/chunk',start=at);quit_app(v)
        v.command('WEB '+base+'/redirect','BROWSER page rendered: '+base+'/second');quit_app(v)
        v.command('WEB '+base+'/chunk','BROWSER page rendered: '+base+'/chunk');quit_app(v)
        v.command('WEB '+base+'/large','BROWSER fetch error: Page exceeds the 2 MiB limit.');quit_app(v)
        v.command('WEB ftp://example.com/','BROWSER fetch error: Use an http:// or https:// URL.');quit_app(v)
        v.command('WEB '+base+'/','BROWSER page rendered: '+base+'/');quit_app(v);results+=['HTTP TCP request and Gumbo HTML5 rendering','keyboard hyperlink and address navigation','HTTP redirect and chunked response','oversized/unsupported-scheme errors and successful retry']
        v.qmp('set_link',{'name':'n','up':False});time.sleep(.3);v.command('NET','Ethernet cable disconnected');v.qmp('set_link',{'name':'n','up':True});time.sleep(6)
        v.command('WEB '+base+'/','BROWSER page rendered: '+base+'/');quit_app(v);results+=['Ethernet cable loss and reconnection']
        v.command('DOOM','DOOM WAD selector ready');v.wait('FILE MANAGER OPEN: .WAD');v.capture('doom-wad-selector');v.pump();at=len(v.log);v.key('esc');v.wait('RUN returned: 0',start=at)
        # WAD picker preselects the bundled IWAD; Enter chooses the game.
        v.command('DOOM','DOOM WAD selector ready');v.wait('FILE MANAGER OPEN: .WAD',start=v.log.rfind('DOOM WAD selector ready'));v.key('ret');v.wait('DOOM loading screen ready',start=v.log.rfind('DOOM WAD selector ready'));v.capture('doom-loading-screen');v.wait('DOOM frame 1',timeout=180);v.capture('doom-after-loading');v.key('f10');v.wait('DOOM exit to BDS-OS');results+=['WAD selector cancel and selected IWAD launch','DOOM loading screen and playable renderer']
        v.command('CHECK','PASS: filesystem integrity')
    finally:v.close();server.shutdown()
    with wave.open(str(wav)) as f:pcm=f.readframes(f.getnframes());rate=f.getframerate()
    samples=struct.unpack('<'+'h'*(len(pcm)//2),pcm);assert min(samples)<-100 and max(samples)>100;results+=['AC97 startup/system PCM chime captured']
    # Boot-only captures isolate system chimes from any game or media audio.
    startup={}
    for backend in ['ac97','hda']:
        sdisk=ROOT/f'build/startup-{backend}-test.img';shutil.copyfile(ROOT/'build/seed.img',sdisk);swav=LOG/f'startup-{backend}-isolated.wav';swav.unlink(missing_ok=True)
        devices=['-device','AC97,audiodev=a'] if backend=='ac97' else ['-device','intel-hda','-device','hda-output,audiodev=a']
        h=VM('startup-'+backend,sdisk,extra=['-audiodev',f'wav,id=a,path={swav}']+devices)
        try:h.wait('SFX startup chime queued');h.wait('DESKTOP ready');time.sleep(.3)
        finally:h.close()
        with wave.open(str(swav)) as f:s=f.readframes(f.getnframes())
        values=struct.unpack('<'+'h'*(len(s)//2),s);assert min(values)<-100 and max(values)>100;startup[backend]=[min(values),max(values)];results+=[backend.upper()+' isolated startup/system PCM chimes captured']
    report={'passed':results,'boot_host_seconds':elapsed,'http_requests':requests,'startup_ranges':startup,'kernel_sha256':tested_kernel,'live_iso_sha256':tested_iso}
    (LOG/'network-desktop-results.json').write_text(json.dumps(report,indent=2));print('PASS',len(results),'network and desktop cases',flush=True)
if __name__=='__main__':main()
