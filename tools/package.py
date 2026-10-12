"""Package the exact verified binaries and their complete project/engine sources."""
import pathlib,zipfile,json,hashlib
ROOT=pathlib.Path(__file__).resolve().parents[1]
report=json.loads((ROOT/'build/logs/results.json').read_text())
for rel,data in report['artifacts'].items():assert hashlib.sha256((ROOT/rel).read_bytes()).hexdigest()==data['sha256'],f'Artifact changed after testing: {rel}'
out=ROOT/'releases/BDS-OS-Ultimate-0.2.zip'
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for name in ['README.md','Makefile','build.ps1','COPYING']:
        z.write(ROOT/name,name)
    for folder in ['legacy','kernel','include','apps','doom','nes','grub','tools','tests','docs','licenses','third_party/doomgeneric','third_party/infones']:
        for p in sorted((ROOT/folder).rglob('*')):
            if p.is_file() and not any(x in ('.git','__pycache__') for x in p.parts):z.write(p,str(p.relative_to(ROOT)))
    for name in report['artifacts']:z.write(ROOT/name,name)
    for p in sorted((ROOT/'build/logs').iterdir()):
        if p.is_file() and p.suffix!='.ppm':z.write(p,str(p.relative_to(ROOT)))
with zipfile.ZipFile(out) as z:assert z.testzip() is None
sha=hashlib.sha256(out.read_bytes()).hexdigest();out.with_suffix('.sha256').write_text(sha+'  '+out.name+'\n')
print(f'Packaged and verified {out} ({out.stat().st_size} bytes)\nSHA256 {sha}')
