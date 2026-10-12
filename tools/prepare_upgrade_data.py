"""Preserve existing VM files while refreshing managed apps/assets on a copy."""
import pathlib,struct,hashlib,json,shutil
from disk import Disk
from desktop_assets import populate
ROOT=pathlib.Path(__file__).resolve().parents[1]
def snapshot(d):
    def path(i):
        e=d.entries[i];return (path(e[1]-1) if e[1] else '')+'/'+e[0].rstrip(b'\0').decode()
    return {path(i):hashlib.sha256(d.data[e[4]*512:e[4]*512+e[3]]).hexdigest() for i,e in enumerate(d.entries) if e[2]==1}
def main():
    raw=(ROOT/'build/vbox-upgrade-source.img').read_bytes();start,blocks=struct.unpack_from('<II',raw,470);assert blocks*512==64*1024*1024
    before=ROOT/'build/vbox-upgrade-data.before.img';assert not before.exists();before.write_bytes(raw[start*512:(start+blocks)*512]);old=snapshot(Disk(before))
    target=ROOT/'build/vbox-upgrade-data.img';assert not target.exists();shutil.copyfile(before,target);d=Disk(target);populate(d);new=snapshot(d);seed=snapshot(Disk(ROOT/'build/seed.img'))
    changed=[];preserved=[]
    for name,sha in old.items():
        assert name in new
        if new[name]!=sha:assert name in seed and new[name]==seed[name],name;changed.append(name)
        else:preserved.append(name)
    roms=[n for n in old if n.upper().endswith(('.NES','.SAV'))];assert len(roms)>=18
    for n in roms:assert old[n]==new[n],n
    backup=ROOT/'build/data.before-upgrade-20261012.img';assert not backup.exists();shutil.copyfile(ROOT/'build/data.img',backup);shutil.copyfile(target,ROOT/'build/data.img')
    source=ROOT/'build/bds-cloud-network-boot.vdi';report={'source_vdi':str(source.relative_to(ROOT)),'source_vdi_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'before_hashes':old,'preserved_files':preserved,'updated_files':changed,'roms_and_saves':roms,'current_vdi':'build/bds-desktop-upgrade-boot.vdi'}
    (ROOT/'build/logs/vbox-upgrade-update.json').write_text(json.dumps(report,indent=2));print('PASS',len(preserved),'files preserved,',len(changed),'managed files updated;',len(roms),'ROMs/saves unchanged')
if __name__=='__main__':main()
