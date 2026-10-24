"""Windows-only benchmark in a new disposable VM, never the user's bds-os VM."""
from pathlib import Path
import argparse,hashlib,json,re,shutil,socket,subprocess,sys,time,uuid
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from disk import Disk
from classicube import fixture
VBOX=Path('C:/Program Files/Oracle/VirtualBox/VBoxManage.exe')
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--quality',choices=['0','1','2'],default='1');ap.add_argument('--scene',choices=['flat','generated'],default='flat');ap.add_argument('--samples',type=int,default=8);a=ap.parse_args()
    name='bds-gaming-test-'+uuid.uuid4().hex[:8];folder=ROOT/'build'/name;folder.mkdir();image=folder/'data.img';vdi=folder/'data.vdi';log=''
    shutil.copyfile(ROOT/'build/seed.img',image);d=Disk(image);blob,_=fixture();d.put('/CLASSIC/maps/benchmark.cw',blob)
    d.put('/CLASSIC/BDS.CFG',a.quality.encode());d.put('/DISPLAY.CFG',b'800 600\n');d.put('/CLASSIC/options.txt',b'defaulttexpack=classicube.zip\nviewdist=32\nfpslimit=LimitNone\nchat-logging=false\nmusicvolume=0\nsoundsvolume=0\n');d.save()
    with socket.socket() as reserve:reserve.bind(('127.0.0.1',0));port=reserve.getsockname()[1]
    def run(*args):
        return subprocess.check_output([str(VBOX),*map(str,args)],text=True,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    created=started=False;s=None;report={}
    try:
        run('convertfromraw',image,vdi,'--format','VDI');run('createvm','--name',name,'--basefolder',folder,'--ostype','Other','--register');created=True
        run('modifyvm',name,'--memory','512','--cpus','1','--vram','64','--graphicscontroller','vboxsvga','--audio','none','--nic1','nat','--nictype1','82540EM','--uart1','0x3F8','4','--uartmode1','tcpserver',port,'--boot1','dvd','--boot2','disk','--paravirtprovider','none')
        run('storagectl',name,'--name','IDE','--add','ide','--controller','PIIX4')
        run('storageattach',name,'--storagectl','IDE','--port','0','--device','0','--type','hdd','--medium',vdi)
        run('storageattach',name,'--storagectl','IDE','--port','1','--device','0','--type','dvddrive','--medium',ROOT/'build/bds-os.iso')
        run('startvm',name,'--type','headless');started=True
        for _ in range(200):
            try:s=socket.create_connection(('127.0.0.1',port),timeout=1);break
            except OSError:time.sleep(.2)
        if not s:raise RuntimeError('VM UART connection unavailable')
        s.settimeout(.2)
        def wait(marker,start=0,timeout=240):
            nonlocal log
            until=time.monotonic()+timeout
            while time.monotonic()<until:
                try:
                    data=s.recv(65536)
                    if not data:raise RuntimeError('VM UART closed')
                    log+=data.decode(errors='replace');(folder/'serial.log').write_text(log)
                except socket.timeout:pass
                if 'FATAL' in log:raise RuntimeError(log[-2000:])
                if marker in log[start:]:return
            raise RuntimeError('Timeout '+marker+'\n'+log[-2000:])
        wait('DESKTOP ready');s.sendall(b'CUBE /CLASSIC/maps/benchmark.cw\r' if a.scene=='flat' else b'CUBE\r');wait('CLASSICUBE frame 100')
        rows=[];cursor=0;last_host=None
        while len(rows)<a.samples:
            wait('CLASSICUBE PERF',cursor);wait('\n',log.index('CLASSICUBE PERF',cursor));now=time.monotonic()
            for m in re.finditer(r'CLASSICUBE PERF frames=(\d+) ms=(\d+) fps10=(\d+) worst_ms=(\d+) size=(\d+x\d+) present_ms=(\d+)\r?\n',log[cursor:]):
                rows.append({'frames':int(m[1]),'guest_ms':int(m[2]),'guest_fps':int(m[3])/10,'worst_ms':int(m[4]),'render_size':m[5],'presentation_ms':int(m[6]),'host_fps':120/(now-last_host) if last_host else None});last_host=now
            cursor=log.rfind('\n')+1
        run('controlvm',name,'screenshotpng',folder/'game.png');run('controlvm',name,'keyboardputscancode','44','c4');wait('RUN returned: 0')
        at=len(log);s.sendall(b'CHECK\r');wait('PASS: filesystem integrity',at)
        report={'emulator':'VirtualBox '+run('--version').strip(),'profile':'new disposable VM, 1 vCPU, 512 MiB, VBoxSVGA 64 MiB, 800x600 desktop','scene':a.scene+' world, view distance 32, uncapped FPS','quality':a.quality,'samples':rows,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest(),'cube_sha256':hashlib.sha256((ROOT/'build/apps/cube.run').read_bytes()).hexdigest()}
        (ROOT/'build/logs'/f'gaming-vbox-{a.quality}-{a.scene}-results.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2),flush=True)
    finally:
        if s:s.close()
        if started:
            try:run('controlvm',name,'poweroff')
            except subprocess.CalledProcessError:pass
        if created:run('unregistervm',name)
        print('Disposable test files retained at',folder,flush=True)
if __name__=='__main__':main()
