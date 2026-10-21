"""Native TLS smoke/security checks against live public HTTPS endpoints."""
import shutil,time,json,hashlib
from interactive import VM,ROOT,LOG
from network_desktop import quit_app

def main():
 disk=ROOT/'build/https-test.img';shutil.copyfile(ROOT/'build/seed.img',disk)
 v=VM('https',disk,extra=['-vga','std','-cpu','max','-nic','user,model=e1000','-rtc','base=2026-10-20T16:00:00'])
 passed=[]
 try:
  v.wait('DESKTOP ready');v.wait('NET DHCP address:')
  for name,url,marker in [('example','https://example.com/','BROWSER page rendered: https://example.com/'),('wikipedia','https://en.wikipedia.org/wiki/Main_Page','BROWSER page rendered: https://en.wikipedia.org/wiki/Main_Page'),('untrusted','https://self-signed.badssl.com/','BROWSER fetch error: HTTPS certificate'),('hostname','https://wrong.host.badssl.com/','BROWSER fetch error: HTTPS certificate'),('expired','https://expired.badssl.com/','BROWSER fetch error: HTTPS certificate')]:
   print('HTTPS test',name,flush=True);v.command('WEB '+url,marker,180);v.capture('https-'+name);passed.append(name);quit_app(v)
 finally:
  v.close();(LOG/'https-results.json').write_text(json.dumps({'passed':passed,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest(),'browser_sha256':hashlib.sha256((ROOT/'build/apps/browser.run').read_bytes()).hexdigest()},indent=2))
if __name__=='__main__':main()
