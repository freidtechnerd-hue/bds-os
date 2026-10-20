"""Compile native image/browser apps with pinned public C source libraries."""
import pathlib,subprocess,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
inc=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-ffile-prefix-map='+str(ROOT)+'=.', '-m32','-Os','-std=gnu11','-ffreestanding','-fno-builtin','-fno-pie','-fno-stack-protector','-mno-sse','-mno-mmx','-mfpmath=387','-ffunction-sections','-fdata-sections','-nostdinc','-isystem',inc,'-I'+str(ROOT/'doom/include'),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'third_party/lodepng'),'-I'+str(ROOT/'third_party/gumbo/src'),'-I'+str(ROOT/'third_party/elk'),'-DLODEPNG_NO_COMPILE_ENCODER','-DLODEPNG_NO_COMPILE_DISK','-DLODEPNG_NO_COMPILE_ANCILLARY_CHUNKS']
apps=sys.argv[1:] or ['image','browser']
for app in apps:
    out=ROOT/'build/utilities'/app;out.mkdir(parents=True,exist_ok=True)
    sources=[ROOT/'apps/utility_libc.c',ROOT/f'apps/{app}.c']
    sources+= [ROOT/'third_party/lodepng/lodepng.c'] if app=='image' else sorted((ROOT/'third_party/gumbo/src').glob('*.c'))+[ROOT/'third_party/elk/elk.c',ROOT/'apps/browser_number.c']
    objects=[]
    for p in sources:
        obj=out/(p.stem+'.o');objects.append(str(obj));print('UTILITY compile',p.name,flush=True)
        subprocess.run(['gcc']+flags+['-c',str(p),'-o',str(obj)],check=True)
    elf=ROOT/f'build/apps/{app}.elf';raw=elf.with_suffix('.bin');run=elf.with_suffix('.run')
    subprocess.run(['ld','-m','elf_i386','--gc-sections','-T',str(ROOT/'apps/link.ld'),'-o',str(elf)]+objects,check=True)
    subprocess.run(['objcopy','-O','binary',str(elf),str(raw)],check=True)
    subprocess.run(['python3',str(ROOT/'tools/runpack.py'),str(elf),str(raw),str(run)],check=True)
