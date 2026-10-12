"""Import user-supplied NES ROMs and a picker manifest into an OFFLINE BDS image."""
import argparse, hashlib, pathlib, sys
from disk import Disk, ROOT

def inspect_rom(data):
    if len(data)<16 or data[:4]!=b'NES\x1a':
        raise ValueError('Expected iNES header')
    nes2=data[7]&12 == 8
    if nes2 and (data[8] or data[9] or (data[10]&15)>7 or data[10]>>4>7 or
                 (data[11]&15)>7 or data[11]>>4>7 or data[12]&3 in (1,3) or
                 data[13] or data[14] or data[15]>1):
        raise ValueError('Unsupported extended NES 2.0 hardware')
    for ram in data[10:12] if nes2 else []:
        n,v=ram&15,ram>>4
        if (64<<n if n else 0)+(64<<v if v else 0)>8192:
            raise ValueError('NES 2.0 RAM exceeds core capacity')
    dirty = not nes2 and any(data[12:16])
    if not dirty and data[7]&3:
        raise ValueError('VS/PlayChoice is not supported')
    mapper = (data[6]>>4) | (0 if dirty else data[7]&240)
    expected = 16 + (512 if data[6]&4 else 0) + data[4]*16384 + data[5]*8192
    if not data[4] or len(data)<expected:
        raise ValueError('Truncated or empty cartridge')
    return mapper, dirty

def import_roms(image, paths):
    # Build every change in memory before saving the image once.
    d = Disk(image)
    d.check()
    run = ROOT/'build/apps/nes.run'
    if not run.exists():
        raise ValueError('Build NES.RUN first: make nes')
    entries = {}
    old = d.find('/NES/ROMS.TXT')
    if old>=0:
        e=d.entries[old]
        for line in bytes(d.data[e[4]*512:e[4]*512+e[3]]).decode().splitlines():
            if '\t' in line:
                p,title=line.split('\t',1);entries[p]=title
    reports = []
    for path in paths:
        path=pathlib.Path(path);data=path.read_bytes();mapper,dirty=inspect_rom(data)
        # A stable short alias fits the 31-byte BDS filename limit.
        short=hashlib.sha256(data).hexdigest()[:12].upper()+'.NES'
        dest='/NES/'+short
        title=path.stem.replace('\t',' ').replace('\r',' ').replace('\n',' ')
        title=title.encode('ascii','replace').decode()[:95]
        entries[dest]=title
        d.put(dest,data)
        reports.append({'source':str(path),'path':dest,'title':title,'mapper':mapper,'dirty_header':dirty})
    if len(entries)>64:
        raise ValueError('ROM picker supports at most 64 cartridges')
    d.put('/NES.RUN',run.read_bytes())
    lines=[p+'\t'+title for p,title in sorted(entries.items(),key=lambda e:e[1].lower())]
    d.put('/NES/ROMS.TXT',('\n'.join(lines)+'\n').encode())
    d.check();d.save()
    for r in reports:
        print(f"{r['path']}  mapper {r['mapper']}  {r['title']}" + (' (dirty iNES header)' if r['dirty_header'] else ''))
    return reports

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('image');ap.add_argument('roms',nargs='+')
    a=ap.parse_args()
    try:import_roms(a.image,a.roms)
    except (OSError,ValueError,AssertionError,StopIteration) as e:sys.exit(str(e))
