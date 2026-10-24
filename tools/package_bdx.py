"""Audit exact tested public images and package the October 23 BDX milestone."""
from pathlib import Path
import argparse, hashlib, json, shutil, struct, subprocess, zipfile, zlib
from disk import Disk

ROOT=Path(__file__).resolve().parents[1]
LOG=ROOT/'build/logs'
TAG='bdx-desktop-x64-preview-2026-10-23'

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def read(disk,path):
    i=disk.find(path);assert i>=0,path
    e=disk.entries[i];return bytes(disk.data[e[4]*512:e[4]*512+e[3]])
def report(name,count,field=None,expected=None):
    r=json.loads((LOG/(name+'-results.json')).read_text())
    assert len(r['passed'])==count,(name,r)
    if field:assert r[field]==expected,(name,'wrong binary')
    return r
def safe_copy(source,dest):
    if dest.exists():assert sha(source)==sha(dest),'Refusing to overwrite a different release artifact'
    else:shutil.copyfile(source,dest)

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--revision',help='Full committed revision; omit for audit only');args=ap.parse_args()
    kernel=ROOT/'build/kernel.elf';iso=ROOT/'build/bds-os-installer.iso';kh=sha(kernel)
    reports={}
    for name,count in [('bdx-theme',4),('modern-desktop',6),('desktop-upgrade',13),('media',6),('classicube',8),('display-settings',11),('settings-tabs',5),('network-desktop',16),('https',5)]:
        reports[name]=report(name,count,'kernel_sha256',kh)
    reports['installer']=report('installer',8,'installer_sha256',sha(iso))
    safety=LOG/'installer-safety-results.txt'
    assert safety.stat().st_mtime>=iso.stat().st_mtime and safety.read_text().startswith('PASS')
    for name,count in [('bdx-sounds',2),('puzzle',2),('legacy-desktop',3)]:reports[name]=report(name,count)
    reports['x64']=report('x64',3,'kernel_sha256',sha(ROOT/'build/x64/kernel.elf'))
    reports['appearance']=report('appearance',3,'kernel_sha256',kh)
    assert reports['appearance']['x64_kernel_sha256']==sha(ROOT/'build/x64/kernel.elf')
    assert reports['x64']['iso_sha256']==sha(ROOT/'build/x64/bds-os-x64.iso')
    assert reports['legacy-desktop']['floppy_sha256']==sha(ROOT/'build/legacy.img')
    benchmark=json.loads((LOG/'gaming-vbox-2-generated-results.json').read_text())
    assert benchmark['kernel_sha256']==kh and benchmark['cube_sha256']==sha(ROOT/'build/apps/cube.run')
    assert len(benchmark['samples'])>=8
    balanced=json.loads((LOG/'gaming-vbox-1-generated-results.json').read_text())
    assert balanced['kernel_sha256']==kh and balanced['cube_sha256']==sha(ROOT/'build/apps/cube.run') and len(balanced['samples'])>=8
    assert all(json.loads((LOG/'bdx-clean-compare.json').read_text()).values())
    assert sha(ROOT/'releases/BDS-OS-GRUB-2.14-2ubuntu2-sources.zip')=='b370b3e4b8b6475bd3ec4dc65944c1061115b491ecc9bd628b0fcee2e6b7f905'
    data=Disk(ROOT/'build/seed.img');data.check();x64=Disk(ROOT/'build/x64/seed.img');x64.check()
    for disk in [data,x64]:
        for e in disk.entries:
            if not e[2]:continue
            name=e[0].rstrip(b'\0').decode().upper()
            assert not name.endswith(('.NES','.SAV','.CW','.VDI','.VMDK','.QCOW2')),name
            assert name not in ['ARIAL.TTF','TIMES.TTF','COMIC.TTF','USER.TTF','USER.PSF','DISPLAY.CFG','SETTINGS.CFG'],name
    apps=list((ROOT/'build/apps').glob('*.run'));assert len(apps)==12
    for p in apps:assert read(data,'/'+p.name.upper())==p.read_bytes(),p.name
    xapps=list((ROOT/'build/x64').glob('*.run'));assert len(xapps)==6
    for p in xapps:assert read(x64,'/'+p.name.upper())==p.read_bytes(),p.name
    for name in ['BDX-ASSETS.TXT','XP-THEME.TXT','FREETYPE-LICENSE.TXT']:
        assert read(data,'/DOCS/'+name) and read(x64,'/DOCS/'+name)
    assert read(data,'/DOCS/DEJAVU-LICENSE.TXT') and read(x64,'/DOCS/FONT-LICENSE.TXT')
    payload=bytearray((ROOT/'build/installer/system.img').read_bytes())
    crc=struct.unpack_from('<I',payload,440)[0];struct.pack_into('<I',payload,440,0);assert zlib.crc32(payload)==crc
    start,blocks=struct.unpack_from('<II',payload,470)
    assert payload[start*512:(start+blocks)*512]==data.data
    extracted=ROOT/'build/public-audit';extracted.mkdir(exist_ok=True)
    for source,name in [('/system.img','system.img'),('/boot/kernel.elf','kernel.elf')]:
        target=extracted/name
        target.unlink(missing_ok=True)
        subprocess.run(['xorriso','-osirrox','on','-indev',str(iso),'-extract',source,str(target)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    assert sha(extracted/'system.img')==sha(ROOT/'build/installer/system.img')
    assert sha(extracted/'kernel.elf')==kh
    result={'tag':TAG,'kernel_sha256':kh,'installer_sha256':sha(iso),'payload_crc32':f'{crc:08x}',
            'apps':{p.name:sha(p) for p in apps},'x64_apps':{p.name:sha(p) for p in xapps},
            'passed_counts':{name:len(r['passed']) for name,r in reports.items()},'benchmark':benchmark,'balanced_benchmark':balanced,
            'public_only':True,'clean_build_reproduced':True}
    (LOG/'bdx-public-audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
    if not args.revision:return
    revision=subprocess.check_output(['git','rev-parse',args.revision],cwd=ROOT,text=True).strip()
    assert len(revision)==40 and revision==args.revision,'Supply a full committed SHA'
    assert not subprocess.check_output(['git','status','--porcelain'],cwd=ROOT).strip(),'Commit the complete source first'
    out=ROOT/'releases'/TAG;out.mkdir(parents=True,exist_ok=True)
    for source,name in [(iso,'BDX-OS-Desktop-Public-2026-10-23.iso'),(ROOT/'build/x64/bds-os-x64.iso','BDX-OS-x64-Preview-2026-10-23.iso'),(ROOT/'build/legacy.img','BDX-OS-8086-2026-10-23.img'),(ROOT/'releases/BDS-OS-GRUB-2.14-2ubuntu2-sources.zip','BDX-OS-GRUB-2.14-2ubuntu2-sources.zip')]:safe_copy(source,out/name)
    with zipfile.ZipFile(out/'BDX-OS-x64-Data-2026-10-23.zip','w',zipfile.ZIP_DEFLATED) as z:
        z.write(ROOT/'build/x64/seed.img','seed.img');z.write(ROOT/'docs/X64.md','README.md')
    subprocess.run(['git','archive','--format=zip','--prefix=BDX-OS/','-o',str(out/'BDX-OS-Source-2026-10-23.zip'),revision],cwd=ROOT,check=True)
    with zipfile.ZipFile(out/'BDX-OS-Verification-2026-10-23.zip','w',zipfile.ZIP_DEFLATED) as z:
        z.writestr('SOURCE-REVISION.txt',revision+'\n')
        for name in reports:z.write(LOG/(name+'-results.json'),name+'-results.json')
        for name in ['bdx-public-audit.json','bdx-clean-compare.json','bdx-clean-build.txt','bdx-clean-reproduce.txt','installer-safety-results.txt','gaming-vbox-2-generated-results.json','gaming-vbox-1-generated-results.json','bdx-final-https-requirements-test.txt','bdx-toolchain.txt']:
            z.write(LOG/name,name)
        for name in ['bdx-ttf-settings.png','bdx-app-minimized.png','x64-live.png','classicube-world.png','display-aspect.png','installed-ide-desktop.png','installed-ahci.png','installed-usb-drive.png']:
            z.write(LOG/name,'screenshots/'+name)
        z.write(ROOT/'docs/RELEASE-2026-10-23.md','RELEASE.md')
    for p in list(out.glob('*.iso'))+list(out.glob('*.img')):
        p.with_suffix(p.suffix+'.sha256').write_text(f'{sha(p)}  {p.name}\n')
    (out/'SHA256SUMS.txt').write_text(''.join(f'{sha(p)}  {p.name}\n' for p in sorted(out.iterdir()) if p.is_file() and p.name!='SHA256SUMS.txt'))
    print('Packaged',out)

if __name__=='__main__':main()
