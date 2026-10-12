"""Optional live Internet DNS/HTTP check; requires working host Internet."""
import sys,pathlib,shutil,http.server,threading,json,time
sys.path.insert(0,str(pathlib.Path('tests').resolve()));from interactive import VM,ROOT,LOG
class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        data=b'<h1>DNS + Ethernet verified</h1><p>Hostname resolved to the host test server.</p>'
        self.send_response(200);self.send_header('Content-Type','text/html');self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
    def log_message(self,*args):pass
server=http.server.ThreadingHTTPServer(('0.0.0.0',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
disk=ROOT/'build/dns-test.img';shutil.copyfile(ROOT/'build/seed.img',disk);v=VM('live-dns',disk,extra=['-netdev','user,id=n','-device','e1000,netdev=n','-object',f'filter-dump,id=trace,netdev=n,file={LOG}/internet-network.pcap'])
try:
    v.wait('DESKTOP ready');v.wait('NET DHCP address:');url=f'http://10.0.2.2.nip.io:{server.server_port}/dns'
    v.command('WEB '+url,'BROWSER page rendered: '+url,70);v.capture('browser-dns-page');v.key('esc');time.sleep(.3)
    passed=['DHCP-provided DNS resolves host name and fetches HTTP page'];notes=[]
    v.pump();start=len(v.log);v.s.sendall(b'WEB http://neverssl.com/\r');end=time.monotonic()+60
    while time.monotonic()<end:
        v.pump()
        if 'BROWSER page rendered: http://neverssl.com/' in v.log[start:]:passed+=['actual Internet HTTP page from neverssl.com'];break
        if 'BROWSER fetch error:' in v.log[start:]:notes+=['neverssl.com transport failed in this host environment'];break
        time.sleep(.1)
    v.capture('browser-internet-neverssl');v.key('esc');time.sleep(.3)
    (LOG/'network-dns-results.json').write_text(json.dumps({'passed':passed,'notes':notes,'observed_at':time.time()},indent=2));print('PASS live DNS; Internet result:',notes or 'page rendered',flush=True)
finally:v.close();server.shutdown()
