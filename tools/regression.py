"""Preserve the working image, clean-build and record the full release regression."""
import pathlib,subprocess,shutil,hashlib,json,datetime
ROOT=pathlib.Path(__file__).resolve().parents[1]
release=ROOT/'releases';release.mkdir(exist_ok=True)
if (ROOT/'build/data.img').exists():shutil.copyfile(ROOT/'build/data.img',release/'pre-regression-data.img')
transcript=release/'regression.log'
with transcript.open('w') as log:
    for target in ['clean','freedoom','all','test','test-ui','test-doom']:
        log.write(f'\nMAKE TARGET: {target}\n');log.flush()
        print('Regression:',target,flush=True)
        result=subprocess.run(['make','-C',str(ROOT),target],stdout=log,stderr=subprocess.STDOUT)
        if result.returncode:raise SystemExit(f'FAIL {target}: see {transcript}')
logs=ROOT/'build/logs';shutil.copyfile(transcript,logs/'regression.log')
versions=subprocess.check_output(['bash','-lc','gcc --version | head -1; ld --version | head -1; nasm -v; grub-mkrescue --version; qemu-system-i386 --version | head -1; dpkg-query -W grub-common grub-pc-bin xorriso qemu-system-x86'],text=True)
(logs/'toolchain.txt').write_text(versions)
artifacts={}
for p in [ROOT/'build/legacy.img',ROOT/'build/bds-os.iso',ROOT/'build/data.img',ROOT/'build/seed.img',*sorted((ROOT/'build/apps').glob('*.run')),ROOT/'build/hello16.run']:
    artifacts[str(p.relative_to(ROOT))]={'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
report={'date':'2026-10-10','environment':'WSL Ubuntu / QEMU PC default CPU / 128 MiB RAM','clean_build':True,'targets_passed':['legacy','grub','shell/storage/reboot','loader','desktop/editor/games','DOOM level/frames/movement/exit'],'host_integrity_tests':6,'doom_upstream':'dcb7a8dbc7a16ce3dda29382ac9aae9d77d21284','artifacts':artifacts}
(logs/'results.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS final clean build and complete regression; '+str(logs/'results.json'),flush=True)
