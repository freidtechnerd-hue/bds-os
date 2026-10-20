"""Freestanding MPEG-1/2, MP2 and AC-3 media player, with pinned source."""
import pathlib,subprocess
root=pathlib.Path(__file__).resolve().parents[1];out=root/'build/media';out.mkdir(parents=True,exist_ok=True)
inc=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m32','-O2','-std=gnu11','-ffreestanding','-fno-builtin','-fno-pie','-fno-stack-protector','-msse2','-mno-mmx','-mfpmath=sse','-ffunction-sections','-fdata-sections','-nostdinc','-isystem',inc,'-DBDS_UTILITY','-Wno-unused-result']
for path in ['media','doom/include','include','third_party/libmpeg2/include','third_party/libmpeg2/libmpeg2','third_party/liba52/include','third_party/pl_mpeg']:flags+=['-I'+str(root/path)]
sources=[root/'media'/p for p in ['start.c','player.c','runtime.c']]+[root/'doom/libc.c']
sources += [root/'third_party/libmpeg2/libmpeg2'/p for p in ['alloc.c','header.c','decode.c','slice.c','motion_comp.c','idct.c','cpu_accel.c','cpu_state.c']]
sources += [root/'third_party/liba52/liba52'/p for p in ['bitstream.c','imdct.c','bit_allocate.c','parse.c','downmix.c']]
objects=[]
for i,p in enumerate(sources):
    obj=out/(str(i)+'-'+p.stem+'.o');objects.append(str(obj));extra=['-mno-sse','-msoft-float'] if p.name=='start.c' else []
    print('MEDIA compile',p.name,flush=True);subprocess.run(['gcc']+flags+extra+['-c',str(p),'-o',str(obj)],check=True)
elf=root/'build/apps/player.elf';raw=elf.with_suffix('.bin')
subprocess.run(['ld','-m','elf_i386','--gc-sections','-T',str(root/'apps/link.ld'),'-o',str(elf)]+objects,check=True)
subprocess.run(['objcopy','-O','binary',str(elf),str(raw)],check=True)
subprocess.run(['python3',str(root/'tools/runpack.py'),str(elf),str(raw),str(elf.with_suffix('.run'))],check=True)
