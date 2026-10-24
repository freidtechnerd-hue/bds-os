"""Measured ClassiCube throughput; never substitutes a menu FPS for gameplay."""
import argparse, hashlib, json, re, shutil, time
from interactive import VM, ROOT, LOG
from classicube import fixture, Disk

def main():
    p=argparse.ArgumentParser();p.add_argument('--label',default='current');p.add_argument('--samples',type=int,default=3)
    p.add_argument('--quality',choices=['0','1','2'],default='1');p.add_argument('--fullscreen',action='store_true');p.add_argument('--size',default='800x600');p.add_argument('--limit',default='LimitNone');args=p.parse_args()
    disk=ROOT/'build/gaming-benchmark.img';shutil.copyfile(ROOT/'build/seed.img',disk)
    d=Disk(disk);blob,_=fixture();d.put('/CLASSIC/maps/benchmark.cw',blob)
    d.put('/CLASSIC/BDS.CFG',args.quality.encode());d.put('/CUBE.RUN',(ROOT/'build/apps/cube.run').read_bytes())
    d.put('/CLASSIC/options.txt',f'defaulttexpack=classicube.zip\nviewdist=32\nfpslimit={args.limit}\nchat-logging=false\nmusicvolume=0\nsoundsvolume=0\n'.encode())
    w,h=map(int,args.size.split('x'));d.put('/DISPLAY.CFG',f'{w} {h}\n'.encode());d.save()
    v=VM('gaming-'+args.label,disk,extra=['-cpu','max','-global','VGA.vgamem_mb=64'])
    try:
        v.wait('DESKTOP ready');v.command('CUBE /CLASSIC/maps/benchmark.cw','CLASSICUBE native')
        v.wait('CLASSICUBE frame 100',180)
        if args.fullscreen:v.key('ctrl-f');v.wait('APP fullscreen enabled')
        started=time.monotonic();cursor=0;rows=[]
        while len(rows)<args.samples:
            v.wait('CLASSICUBE PERF',180,cursor)
            for m in re.finditer(r'CLASSICUBE PERF frames=(\d+) ms=(\d+) fps10=(\d+) worst_ms=(\d+) size=(\d+x\d+)',v.log[cursor:]):
                rows.append({'frames':int(m[1]),'guest_ms':int(m[2]),'guest_fps':int(m[3])/10,'worst_frame_ms':int(m[4]),'render_size':m[5]})
            cursor=len(v.log)
        elapsed=time.monotonic()-started
        v.capture('gaming-'+args.label);v.key('f10');v.wait('RUN returned: 0');v.command('CHECK','PASS: filesystem integrity')
        report={'label':args.label,'emulator':'QEMU TCG, single vCPU, -cpu max, 128 MiB','display':args.size,'fullscreen':args.fullscreen,'frame_limit':args.limit,'scene':'64x32x64 textured flat world, view distance 32, fixed spawn','samples':rows,'host_observation_seconds':elapsed,'kernel_sha256':hashlib.sha256((ROOT/'build/kernel.elf').read_bytes()).hexdigest(),'cube_sha256':hashlib.sha256((ROOT/'build/apps/cube.run').read_bytes()).hexdigest()}
        (LOG/('gaming-'+args.label+'-results.json')).write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2),flush=True)
    finally:v.close()

if __name__=='__main__':main()
