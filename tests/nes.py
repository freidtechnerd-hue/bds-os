"""Native NES acceptance: synthetic cartridge, validation, input, saves, user ROMs."""
import argparse, json, pathlib, shutil, struct, sys, time
from interactive import VM, LOG, ROOT, pixel
sys.path.insert(0,str(ROOT/'tools'))
from disk import Disk

def cartridge():
    # Original tiny 6502 program: blue backdrop; hold A to turn it red.
    # Increment battery RAM once at power-on so reboot persistence is observable.
    code=bytearray();labels={};fix=[]
    def emit(*b):code.extend(b)
    def label(n):labels[n]=len(code)
    def branch(op,n):emit(op,0);fix.append((len(code)-1,n,'rel'))
    def jump(n):emit(0x4c,0,0);fix.append((len(code)-2,n,'abs'))
    emit(0x78,0xd8,0xa2,0xff,0x9a,0xa9,0,0x8d,0,0x20,0x8d,1,0x20,0xee,0,0x60)
    label('loop');label('vblank');emit(0x2c,2,0x20);branch(0x10,'vblank')
    emit(0xa9,1,0x8d,0x16,0x40,0xa9,0,0x8d,0x16,0x40,0xad,0x16,0x40,0x29,1)
    branch(0xf0,'blue');emit(0xa2,0x16);jump('color')
    label('blue');emit(0xa2,0x21);label('color')
    emit(0xa9,0x3f,0x8d,6,0x20,0xa9,0,0x8d,6,0x20,0x8a,0x8d,7,0x20,0xa9,0x0a,0x8d,1,0x20)
    jump('loop')
    for pos,n,kind in fix:
        if kind=='rel':code[pos]=(labels[n]-pos-1)&255
        else:struct.pack_into('<H',code,pos,0x8000+labels[n])
    prg=bytearray([0xea]*16384);prg[:len(code)]=code
    for offset in [16378,16380,16382]:struct.pack_into('<H',prg,offset,0x8000)
    return b'NES\x1a'+bytes([1,0,2,0])+bytes(8)+prg

def read_file(d,p):
    i=d.find(p);assert i>=0,p;e=d.entries[i]
    return bytes(d.data[e[4]*512:e[4]*512+e[3]])

def platform_test():
    disk=ROOT/'build/nes-platform-test.img';shutil.copyfile(ROOT/'build/seed.img',disk)
    d=Disk(disk);rom=cartridge();d.put('/NES/TEST.NES',rom)
    d.put('/NES/BAD.NES',b'not a cartridge');d.put('/NES/SHORT.NES',rom[:20])
    extended=bytearray(rom);extended[7]=8;extended[8]=1;d.put('/NES/EXT.NES',extended)
    bad=bytearray(rom);bad[6]=0xe0;bad[7]=0xf0;d.put('/NES/MAPPER.NES',bad)
    d.put('/NES/ROMS.TXT',b'/NES/TEST.NES\tOriginal BDS test cartridge\n');d.save()
    for boot in range(2):
        v=VM('nes-platform-'+str(boot),disk)
        try:
            v.wait('DESKTOP ready')
            if boot==0:
                for p,marker in [('BAD','missing or short ROM'),('SHORT','truncated or empty ROM'),('MAPPER','unsupported'),('EXT','unsupported extended NES 2.0 hardware')]:
                    v.command('NES /NES/'+p+'.NES',marker);v.wait('RUN returned: 1');time.sleep(.2)
                v.command('NES /NES/MISSING.NES','missing or short ROM');v.wait('RUN returned: 1')
                # Game Center keyboard entry opens the native ROM picker.
                v.key('f5');time.sleep(.2);v.key('n');v.wait('NES library ready');v.capture('nes-library')
                v.key('ret');v.wait('NES native emulation started')
            else:
                v.command('NES /NES/TEST.NES','NES battery loaded')
            v.wait('NES frame 120');v.capture('nes-test-blue')
            assert pixel(LOG/'nes-test-blue.ppm',350,300)==bytes.fromhex('38b8f8')
            v.key('x',1800);v.wait('NES controller: 1');time.sleep(.3);v.capture('nes-test-red')
            assert pixel(LOG/'nes-test-red.ppm',350,300)==bytes.fromhex('d82800')
            time.sleep(1.6);v.key('r');v.wait('NES reset');time.sleep(.2)
            v.key('esc');v.wait('NES exit to BDS-OS');v.wait('RUN returned: 0')
            v.key('f1');time.sleep(.2)
            v.command('VER','BDX-OS Ultimate 0.2');v.command('CHECK','PASS: filesystem integrity')
        finally:v.close()
        saved=read_file(Disk(disk),'/NES/TEST.SAV')
        assert len(saved)==8192 and saved[0]==2*(boot+1),(boot,saved[:8])
        if boot==0:
            d=Disk(disk);compatible=bytearray(rom);compatible[7]=8;compatible[15]=1
            d.put('/NES/TEST.NES',compatible);d.save()
    # File Manager recognizes .NES: enter NES directory and open TEST.NES.
    v=VM('nes-file-manager',disk)
    try:
        v.wait('DESKTOP ready');v.command('CD /NES','BDX> CD /NES');time.sleep(.3)
        v.key('f2');time.sleep(.3);v.key('ret');v.wait('NES ROM: V0:/NES/TEST.NES');v.wait('NES frame 1')
        v.key('f10');v.wait('NES exit to BDS-OS');v.wait('RUN returned: 0')
    finally:v.close()
    print('PASS NES native graphics/controller, bad ROM recovery, picker, file manager, reset, SRAM across reboot, exit',flush=True)

def rom_test(report_path):
    reports=json.loads(pathlib.Path(report_path).read_text())
    disk=ROOT/'build/nes-rom-test.img';shutil.copyfile(ROOT/'build/data.img',disk)
    v=VM('nes-roms',disk);results=[]
    try:
        v.wait('DESKTOP ready')
        for i,r in enumerate(reports):
            capture='nes-rom-'+pathlib.PurePosixPath(r['path']).stem.lower()
            start=len(v.log);v.command('NES '+r['path'],'NES native emulation started',60)
            v.wait('NES frame 120',90,start=start)
            for k in ['ret','spc','ret']:
                v.key(k,250);time.sleep(.35)
            # Some cartridges fade to a flat color while entering a level.
            # Wait for that transition rather than classify one frame as blank.
            ppm=LOG/(capture+'.ppm');end=time.monotonic()+15
            while True:
                time.sleep(.5);v.qmp('screendump',{'filename':str(ppm)})
                head,data=ppm.read_bytes().split(b'\n255\n',1)
                w,h=map(int,head.split()[1:3]);colors=set()
                for y in range(60,540,4):
                    for x in range(144,656,4):
                        if 400<=x<412 and 300<=y<319:continue # cursor overlay
                        p=(y*w+x)*3;colors.add(data[p:p+3])
                if len(colors)>3 or time.monotonic()>end:break
            shot=v.capture(capture)
            nonblank=len(colors)>3
            v.key('right',700);v.key('x',700);time.sleep(.8)
            v.key('esc');v.wait('NES exit to BDS-OS',60,start);v.wait('RUN returned: 0',60,start)
            results.append({**r,'status':'PASS load, 120 frames, start/input, nonblank screen, exit' if nonblank else 'FAIL blank screen','colors':len(colors),'capture':str(shot.relative_to(ROOT))})
            print('PASS' if nonblank else 'FAIL blank screen',r['title'],'mapper',r['mapper'],flush=True);time.sleep(.2)
        v.command('CHECK','PASS: filesystem integrity')
    finally:
        v.close();(LOG/'nes-results.json').write_text(json.dumps(results,indent=2)+'\n')
    assert len(results)==len(reports)
    assert all(r['status'].startswith('PASS') for r in results), 'See build/logs/nes-results.json for incompatible cartridges'

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--rom-report');args=ap.parse_args()
    platform_test()
    if args.rom_report:rom_test(args.rom_report)
