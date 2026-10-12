"""Fetch the pinned freely redistributable IWAD with its license and credits."""
import hashlib,pathlib,urllib.request,zipfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
URL='https://github.com/freedoom/freedoom/releases/download/v0.13.0/freedoom-0.13.0.zip'
SHA256='3f9b264f3e3ce503b4fb7f6bdcb1f419d93c7b546f4df3e874dd878db9688f59'
assets=ROOT/'build/assets';assets.mkdir(parents=True,exist_ok=True);archive=assets/'freedoom-0.13.0.zip'
if not archive.exists():
    print('Download',URL,flush=True);urllib.request.urlretrieve(URL,archive)
assert hashlib.sha256(archive.read_bytes()).hexdigest()==SHA256,'Freedoom archive hash mismatch'
with zipfile.ZipFile(archive) as z:
    for name in ['freedoom1.wad','COPYING.txt','CREDITS.txt','CREDITS-MUSIC.txt']:
        (assets/name).write_bytes(z.read('freedoom-0.13.0/'+name))
print('Freedoom 0.13.0 verified and extracted, including redistribution notices')
