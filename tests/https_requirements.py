"""TLS fails closed without hardware entropy or a valid date; HTTP still works."""
import http.server,threading,shutil,json
from interactive import VM,ROOT,LOG
from network_desktop import quit_app
class Server(http.server.BaseHTTPRequestHandler):
 def log_message(self,*a):pass
 def do_GET(self):
  body=b'<h1>HTTP recovery works</h1>';self.send_response(200);self.send_header('Content-Type','text/html');self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
def main():
 s=http.server.ThreadingHTTPServer(('0.0.0.0',0),Server);threading.Thread(target=s.serve_forever,daemon=True).start();passed=[]
 try:
  for name,cpu,date,expected in [('entropy','pentium3','2026-10-21T16:00:00','HTTPS needs CPU RDRAND'),('clock','max','2001-01-01T00:00:00','HTTPS needs a valid UTC')]:
   disk=ROOT/f'build/https-{name}.img';shutil.copyfile(ROOT/'build/seed.img',disk)
   v=VM('https-'+name,disk,extra=['-cpu',cpu,'-rtc','base='+date,'-nic','user,model=e1000'])
   try:
    v.wait('DESKTOP ready');v.command('WEB https://10.0.2.2/','BROWSER fetch error: '+expected,120);quit_app(v)
    url=f'http://10.0.2.2:{s.server_port}/';v.command('WEB '+url,'BROWSER page rendered: '+url);quit_app(v);passed.append(name)
   finally:v.close()
 finally:s.shutdown()
 (LOG/'https-requirements-results.json').write_text(json.dumps({'passed':passed},indent=2))
 print('PASS entropy/date failures and subsequent HTTP recovery',flush=True)
if __name__=='__main__':main()
