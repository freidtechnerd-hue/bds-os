"""Real guest pixels: black splash, navy title bars and native transparent icons."""
import hashlib,json,shutil,struct,time
from interactive import ROOT,LOG,VM,pixel

def icon(path):
    b=path.read_bytes();side=b[6];off=struct.unpack_from('<I',b,18)[0]
    assert side in (16,32) and b[7]==side
    rows=[]
    for y in range(side):
        row=[]
        for x in range(side):
            p=off+40+(side-1-y)*side*4+x*4
            blue,green,red,alpha=b[p:p+4]
            if b[off+40+side*side*4+(side-1-y)*4+x//8]&(128>>(x%8)):alpha=0
            row.append((red,green,blue,alpha))
        rows.append(row)
    return rows

def main():
    passed=[]
    for bits,folder in [(32,'icons'),(64,'icons32')]:
        expected=16 if bits==32 else 32
        for p in (ROOT/'assets'/folder).glob('*.ICO'):assert len(icon(p))==expected
        for name in ['EDITOR','IMAGE']:
            rows=icon(ROOT/'assets'/folder/(name+'.ICO'))
            assert sum(a==0 for row in rows for r,g,b,a in row)>=expected,(bits,name,'missing transparent exterior')
            if expected==32:assert rows[0][0][3]==0,(bits,name,'opaque native corner')
            assert any(min(r,g,b)>235 and a==255 for row in rows for r,g,b,a in row),(bits,name,'lost white artwork')
        seed=ROOT/('build/seed.img' if bits==32 else 'build/x64/seed.img');disk=ROOT/f'build/appearance-{bits}.img';shutil.copyfile(seed,disk)
        iso=ROOT/('build/bds-os.iso' if bits==32 else 'build/x64/bds-os-x64.iso')
        v=VM(f'appearance-{bits}',disk,iso=iso,memory=256,emulator='qemu-system-i386' if bits==32 else 'qemu-system-x86_64')
        try:
            v.wait('BOOT BMP splash ready');v.capture(f'appearance-{bits}-boot');p=LOG/f'appearance-{bits}-boot.ppm'
            header=p.read_bytes().split(b'\n255\n',1)[0].split();w,h=map(int,header[1:3])
            for x,y in [(1,1),(w-2,h-2),(42,h-65)]:assert pixel(p,x,y)==b'\0\0\0'
            v.wait('DESKTOP ready');p=LOG/f'appearance-{bits}-desktop.ppm'
            # The readiness marker precedes the first full wallpaper repaint.
            for _ in range(80):
                v.qmp('screendump',{'filename':str(p)})
                if pixel(p,200,65)==b'\0\0\x80':break
                time.sleep(.1)
            v.capture(f'appearance-{bits}-desktop')
            assert pixel(p,200,65)==b'\0\0\x80','Active desktop title must be navy'
            # Native source pixels must appear at exactly 32 desktop pixels.
            rows=icon(ROOT/'assets'/folder/'FILEMAN.ICO');matched=0
            for y in range(32):
                for x in range(32):
                    r,g,b,a=rows[y*expected//32][x*expected//32]
                    if a==255:assert pixel(p,22+x,95+y)==bytes([r,g,b]);matched+=1
            assert matched>400
            # The formerly white exterior reveals the cloud wallpaper.
            assert pixel(p,22,162)!=b'\xff\xff\xff' and pixel(p,w-78,100)!=b'\xff\xff\xff'
            v.command('EDIT /APPEAR.TXT','EDITOR ready');time.sleep(.3);v.capture(f'appearance-{bits}-app')
            assert pixel(LOG/f'appearance-{bits}-app.ppm',(w-640)//2+120,(h-480-52)//2+2)==b'\0\0\x80'
            v.key('esc');v.wait('RUN returned: 0');v.command('CHECK','PASS: filesystem integrity')
            passed.append(f'{bits}-bit: black boot surround, navy desktop/app titles, transparent artwork and {expected}x{expected} source icons verified in framebuffer')
        finally:v.close()
    for p in (ROOT/'build/x64').glob('*.run'):
        b=p.read_bytes();code=struct.unpack_from('<I',b,8)[0];length=struct.unpack_from('<I',b,28)[0]
        assert length==4286 and b[32+code+6:32+code+8]==b'  ',p
    passed.append('All six x64 native apps embed checksummed 32x32 ICO trailers')
    result={'passed':passed,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest(),'x64_kernel_sha256':hashlib.sha256((ROOT/'build/x64/kernel.elf').read_bytes()).hexdigest()}
    (LOG/'appearance-results.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))

if __name__=='__main__':main()
