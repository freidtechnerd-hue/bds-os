"""Native ClassiCube acceptance: real input, saved NBT worlds and cold boot."""
import gzip,hashlib,io,json,shutil,struct,sys,time
from interactive import VM,ROOT,LOG
sys.path.insert(0,str(ROOT/'tools'))
from disk import Disk

def tag(kind,name,data):
    b=name.encode();return bytes([kind])+struct.pack('>H',len(b))+b+data

def fixture():
    blocks=bytes([1])*64*64*7+bytes([2])*64*64+bytes(64*64*24)
    short=lambda n,v:tag(2,n,struct.pack('>H',v))
    spawn=short('X',32)+short('Y',10)+short('Z',32)+tag(1,'H',bytes([0]))+tag(1,'P',bytes([50]))+b'\0'
    data=tag(1,'FormatVersion',b'\1')+tag(7,'UUID',struct.pack('>I',16)+bytes(16))
    data+=short('X',64)+short('Y',32)+short('Z',64)+tag(10,'Spawn',spawn)
    data+=tag(7,'BlockArray',struct.pack('>I',len(blocks))+blocks)+b'\0'
    return gzip.compress(tag(10,'ClassicWorld',data),mtime=0),blocks

def world(image,name):
    d=Disk(image);i=d.find('/CLASSIC/maps/'+name+'.cw');assert i>=0,name
    e=d.entries[i];f=io.BytesIO(gzip.decompress(bytes(d.data[e[4]*512:e[4]*512+e[3]])))
    def num(fmt):return struct.unpack('>'+fmt,f.read(struct.calcsize('>'+fmt)))[0]
    def string():return f.read(num('H')).decode()
    def payload(k):
        if k in (1,2,3,4,5,6):return num({1:'b',2:'h',3:'i',4:'q',5:'f',6:'d'}[k])
        if k==7:return f.read(num('i'))
        if k==8:return string()
        if k==9:
            t=num('B');return [payload(t) for _ in range(num('i'))]
        if k==10:
            out={}
            while True:
                t=num('B')
                if not t:return out
                n=string();out[n]=payload(t)
        if k in (11,12):return [num('i' if k==11 else 'q') for _ in range(num('i'))]
        raise AssertionError(k)
    assert num('B')==10;assert string()=='ClassicWorld';return payload(10)

def key(v,k,delay=1):v.key(k);time.sleep(delay)

def click(v,x,y):
    # Keep PS/2 packets within signed-byte range; the port polls during rendering.
    for _ in range(12):v.mouse(-120,-120);time.sleep(.08)
    while x or y:
        dx=min(x,80);dy=min(y,80);v.mouse(dx,dy);x-=dx;y-=dy;time.sleep(.08)
    time.sleep(.5);v.mouse(down=True);time.sleep(.8);v.mouse(down=False);time.sleep(1)

def save(v,name,paused=False):
    if not paused:key(v,'esc',2)
    click(v,528,362)
    # Serial follows the same kernel event queue as PS/2 character input.
    v.s.sendall(b'\x08'*24+name.encode());time.sleep(1)
    start=len(v.log);key(v,'ret');v.wait('CLASSICUBE saved: /CLASSIC/maps/'+name+'.cw',60,start)

def main():
    cube_hash=hashlib.sha256((ROOT/'build/apps/cube.run').read_bytes()).hexdigest()
    disk=ROOT/'build/classicube-test.img';shutil.copyfile(ROOT/'build/seed.img',disk)
    blob,blocks=fixture();d=Disk(disk);d.put('/CLASSIC/maps/fixture.cw',blob);d.save();passed=[]
    v=VM('classicube',disk)
    try:
        v.wait('DESKTOP ready');v.command('CUBE /CLASSIC/maps/fixture.cw','CLASSICUBE native')
        v.wait('CLASSICUBE frame 30',180);v.capture('classicube-world')
        passed.append('Native executable loads a ClassicWorld file and renders textured terrain')
        for quality in [2,0,1]:
            at=len(v.log);key(v,'f9',2);v.wait('CLASSICUBE quality changed',start=at)
            d=Disk(disk);e=d.entries[d.find('/CLASSIC/BDS.CFG')];assert d.data[e[4]*512:e[4]*512+e[3]]==str(quality).encode()
            v.capture('classicube-quality-'+str(quality))
        passed.append('F9 resizes the live renderer through all three quality levels and saves each choice')
        v.mouse(down=True);time.sleep(.8);v.mouse(down=False);time.sleep(1)
        save(v,'removed');removed=world(disk,'removed');assert removed['BlockArray']!=blocks
        assert removed['BlockArray'].count(0)>blocks.count(0)
        passed.append('Left-click removes blocks; Save level writes valid gzip/NBT data')
        key(v,'esc',2);v.mouse(60,-70);time.sleep(2)
        v.qmp('input-send-event',{'events':[{'type':'btn','data':{'button':'right','down':True}}]});time.sleep(.8)
        v.qmp('input-send-event',{'events':[{'type':'btn','data':{'button':'right','down':False}}]});time.sleep(1)
        v.capture('classicube-placed');save(v,'placed');placed=world(disk,'placed')
        assert placed['BlockArray'].count(0)<removed['BlockArray'].count(0)
        passed.append('Right-click places blocks and the changed world persists')
        key(v,'esc',2);v.mouse(60,-80);v.key('w',1600);time.sleep(3)
        save(v,'moved');moved=world(disk,'moved')
        assert any(moved['Spawn'][a]!=placed['Spawn'][a] for a in ('X','Y','Z'))
        passed.append('Relative mouse look and WASD movement change the saved player position')
        key(v,'ctrl-f');v.wait('APP fullscreen enabled');v.capture('classicube-fullscreen')
        key(v,'ctrl-f');v.wait('APP windowed enabled');key(v,'f10');v.wait('RUN returned: 0')
        v.command('CHECK','PASS: filesystem integrity');passed.append('Ctrl+F toggles fullscreen; F10 returns with intact filesystem')
    finally:v.close()
    v=VM('classicube-cold',disk)
    try:
        v.wait('DESKTOP ready');key(v,'f5');key(v,'c');v.wait('CLASSICUBE frame 30',180)
        key(v,'esc',2);click(v,528,322);v.capture('classicube-load-list')
        # Alphabetical list: fixture, moved, placed, removed.
        click(v,400,282);time.sleep(3);v.capture('classicube-reloaded')
        click(v,400,527) # Done returns to the pause menu after loading.
        save(v,'reloaded',paused=True);reloaded=world(disk,'reloaded')
        assert reloaded['BlockArray']==moved['BlockArray']
        assert reloaded['Spawn']==moved['Spawn']
        passed.append('Game Center generates a new world; Load level enumerates and restores a saved world after cold boot')
        key(v,'f10');v.wait('RUN returned: 0');key(v,'f1');v.command('CHECK','PASS: filesystem integrity')
    finally:v.close()
    Disk(disk).check()
    v=VM('classicube-old-cpu',disk,extra=['-cpu','pentium3'])
    try:
        v.wait('DESKTOP ready');v.command('CUBE','ClassiCube requires an SSE2-capable CPU.')
        v.wait('RUN returned: 4294967295');v.command('CHECK','PASS: filesystem integrity')
        passed.append('CPU without SSE2 rejects the game cleanly and returns to a working desktop')
    finally:v.close()
    assert cube_hash==hashlib.sha256((ROOT/'build/apps/cube.run').read_bytes()).hexdigest(),'Executable changed during test'
    result={'passed':passed,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest(),'cube_sha256':cube_hash}
    (LOG/'classicube-results.json').write_text(json.dumps(result,indent=2))
    for p in passed:print('PASS',p,flush=True)

if __name__=='__main__':main()
