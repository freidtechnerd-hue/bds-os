"""BDSFS01 host tools. Mutate only an offline image, never a running QEMU disk."""
import argparse, pathlib, struct
ROOT=pathlib.Path(__file__).resolve().parents[1]
FMT='<32sIIIII12s'
class Disk:
    def __init__(self,path):
        self.path=pathlib.Path(path);self.data=bytearray(self.path.read_bytes())
        assert self.data[:8]==b'BDSFS01\0','Invalid superblock'
        self.blocks,self.count=struct.unpack_from('<II',self.data,8)
        self.entries=[list(struct.unpack_from(FMT,self.data,512+i*64)) for i in range(128)]
    def find(self,path):
        parent=0;i=-1
        for name in path.replace('\\','/').strip('/').split('/'):
            i=next((j for j,e in enumerate(self.entries) if e[2] and e[1]==parent and e[0].rstrip(b'\0').decode().lower()==name.lower()),-1)
            if i<0:return -1
            parent=i+1
        return i
    def put(self,path,data=None):
        parts=path.strip('/').split('/');parent=0
        for n,name in enumerate(parts):
            assert 0<len(name.encode())<32 and name not in ('.','..')
            i=self.find('/'.join(parts[:n+1]));last=n==len(parts)-1
            if i<0:i=next(j for j,e in enumerate(self.entries) if not e[2])
            if not last or data is None:
                assert self.entries[i][2] in (0,2)
                self.entries[i]=[name.encode().ljust(32,b'\0'),parent,2,0,0,0,b'\0'*12]
            else:
                blocks=max(8,(len(data)+511)//512);start=17
                while True:
                    overlaps=[e for e in self.entries if e[2]==1 and start<e[4]+e[5] and e[4]<start+blocks]
                    if not overlaps:break
                    start=max(e[4]+e[5] for e in overlaps)
                assert start+blocks<=self.blocks,'Disk full'
                self.data[start*512:(start+blocks)*512]=data.ljust(blocks*512,b'\0')
                self.entries[i]=[name.encode().ljust(32,b'\0'),parent,1,len(data),start,blocks,b'\0'*12]
            parent=i+1
    def save(self):
        for i,e in enumerate(self.entries):struct.pack_into(FMT,self.data,512+i*64,*e)
        self.path.write_bytes(self.data)
    def check(self):
        assert len(self.data)==self.blocks*512 and self.count==128
        names=set();extents=[]
        for i,e in enumerate(self.entries):
            name,parent,typ,size,start,cap,_=e
            if not typ:continue
            assert typ in (1,2) and b'\0' in name and name[0]
            assert parent<=128 and (not parent or self.entries[parent-1][2]==2)
            seen={i+1};p=parent
            while p:assert p not in seen;seen.add(p);p=self.entries[p-1][1]
            key=(parent,name.lower());assert key not in names;names.add(key)
            if typ==1:
                assert 17<=start<self.blocks and size<=cap*512 and start+cap<=self.blocks
                for a,b in extents:assert start>=b or start+cap<=a
                extents.append((start,start+cap))
        print(f'PASS BDSFS integrity: {len(names)} entries, {len(extents)} file extents')
def create(path):
    data=bytearray(64*1024*1024);data[:8]=b'BDSFS01\0';struct.pack_into('<II',data,8,len(data)//512,128);pathlib.Path(path).write_bytes(data)
    d=Disk(path);d.put('/README.TXT',b'BDS-OS Ultimate: persistent BDSFS01 IDE disk.\nUse HELP; F1 opens the desktop.\n')
    for p in sorted((ROOT/'build/apps').glob('*.run')):d.put('/'+p.name.upper(),p.read_bytes())
    d.put('/GAMES');d.put('/DOCS');d.save();d.check()
    wad=ROOT/'build/assets/freedoom1.wad'
    if wad.exists():
        d.put('/FREEDOOM1.WAD',wad.read_bytes())
        for name in ['COPYING.txt','CREDITS.txt','CREDITS-MUSIC.txt']:
            d.put('/DOCS/'+name,(ROOT/'build/assets'/name).read_bytes())
        d.save();d.check()
    from desktop_assets import generate,populate
    generate();populate(d)
if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('command',choices=['create','check','list','import','export']);ap.add_argument('image');ap.add_argument('args',nargs='*');a=ap.parse_args()
    if a.command=='create':create(a.image)
    else:
        d=Disk(a.image)
        if a.command=='check':d.check()
        elif a.command=='list':
            for i,e in enumerate(d.entries):
                if e[2]:print(i,e[0].rstrip(b'\0').decode(), 'parent',e[1], 'bytes',e[3])
        elif a.command=='import':d.put(a.args[1],pathlib.Path(a.args[0]).read_bytes());d.save();d.check()
        elif a.command=='export':
            i=d.find(a.args[0]);assert i>=0 and d.entries[i][2]==1;e=d.entries[i];pathlib.Path(a.args[1]).write_bytes(d.data[e[4]*512:e[4]*512+e[3]])
