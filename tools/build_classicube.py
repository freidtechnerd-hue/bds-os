"""Compile the pinned ClassiCube sources as a freestanding BDS .RUN."""
import pathlib,subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1];src=ROOT/'third_party/classicube/src';out=ROOT/'build/classicube';out.mkdir(parents=True,exist_ok=True)
inc=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m32','-O3','-std=gnu99','-ffreestanding','-fno-builtin','-fno-pie','-fno-stack-protector','-msse2','-mno-mmx','-mfpmath=sse','-fno-math-errno','-ffunction-sections','-fdata-sections','-nostdinc','-isystem',inc,'-I'+str(src),'-I'+str(ROOT/'doom/include'),'-I'+str(ROOT/'include'),'-include',str(ROOT/'classicube/config.h'),'-DBDS_UTILITY','-Wno-pointer-sign']
objects=[];headers=[*src.glob('*.h'),ROOT/'include/bds.h',ROOT/'doom/include/libc.h',ROOT/'classicube/config.h',pathlib.Path(__file__)];stamp=max(p.stat().st_mtime for p in headers)
for p in [*sorted(src.glob('*.c')),ROOT/'classicube/platform.c',ROOT/'classicube/window.c',ROOT/'classicube/start.c',ROOT/'doom/libc.c']:
    obj=out/(p.stem+'.o');objects.append(str(obj))
    if not obj.exists() or obj.stat().st_mtime<max(p.stat().st_mtime,stamp):
        compile_flags=flags+(['-mno-sse','-msoft-float'] if p.name=='start.c' else [])
        print('ClassiCube compile',p.name,flush=True);subprocess.run(['gcc']+compile_flags+['-c',str(p),'-o',str(obj)],check=True)
elf=ROOT/'build/apps/cube.elf';raw=elf.with_suffix('.bin');run=elf.with_suffix('.run')
subprocess.run(['ld','-m','elf_i386','--gc-sections','-T',str(ROOT/'apps/link.ld'),'-o',str(elf)]+objects,check=True)
subprocess.run(['objcopy','-O','binary',str(elf),str(raw)],check=True)
subprocess.run(['python3',str(ROOT/'tools/runpack.py'),str(elf),str(raw),str(run)],check=True)
