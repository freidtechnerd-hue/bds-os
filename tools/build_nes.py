"""Compile pinned InfoNES directly into a freestanding BDS i386 application."""
import pathlib, subprocess
ROOT = pathlib.Path(__file__).resolve().parents[1]
src = ROOT / 'third_party/infones/src'
out = ROOT / 'build/nes'
out.mkdir(parents=True, exist_ok=True)
(ROOT / 'build/apps').mkdir(parents=True, exist_ok=True)
flags = ['-m32', '-O2', '-std=gnu++11', '-ffreestanding', '-fno-builtin', '-fno-pie',
         '-fno-stack-protector', '-fno-exceptions', '-fno-rtti',
         '-fno-threadsafe-statics', '-mno-sse', '-mno-mmx', '-mfpmath=387',
         '-ffunction-sections', '-fdata-sections', '-nostdinc++',
         '-I' + str(ROOT/'include'), '-I' + str(src), '-Wno-write-strings',
         '-Wno-narrowing']
sources = [src/(name+'.cpp') for name in ['InfoNES', 'K6502', 'InfoNES_Mapper', 'InfoNES_pAPU']]
sources.append(ROOT/'nes/platform.cpp')
headers = list(src.glob('*.h')) + list((src/'mapper').glob('*.cpp')) + [ROOT/'include/bds.h', pathlib.Path(__file__)]
header_time = max(p.stat().st_mtime for p in headers)
objects = []
for p in sources:
    obj = out/(p.stem+'.o')
    objects.append(str(obj))
    if not obj.exists() or obj.stat().st_mtime < max(p.stat().st_mtime, header_time):
        print('NES compile', p.name, flush=True)
        subprocess.run(['g++', *flags, '-c', str(p), '-o', str(obj)], check=True)
elf = ROOT/'build/apps/nes.elf'
subprocess.run(['ld', '-m', 'elf_i386', '--gc-sections', '-T', str(ROOT/'apps/link.ld'), '-o', str(elf), *objects], check=True)
subprocess.run(['objcopy', '-O', 'binary', str(elf), str(elf.with_suffix('.bin'))], check=True)
subprocess.run(['python3', str(ROOT/'tools/runpack.py'), str(elf), str(elf.with_suffix('.bin')), str(elf.with_suffix('.run'))], check=True)
