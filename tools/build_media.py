"""Freestanding MPEG-1/2, MP2 and AC-3 media player, with pinned source."""
import pathlib,subprocess
root=pathlib.Path(__file__).resolve().parents[1];out=root/'build/media';out.mkdir(parents=True,exist_ok=True)
inc=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m32','-O2','-std=gnu11','-ffreestanding','-fno-builtin','-fno-pie','-fno-stack-protector','-msse2','-mno-mmx','-mfpmath=sse','-ffunction-sections','-fdata-sections','-nostdinc','-isystem',inc,'-DBDS_UTILITY','-DHAVE_CONFIG_H','-DNDEBUG','-Wno-unused-result']
for path in ['media/compat','build/media/include','media','doom/include','include','third_party/libmpeg2/include','third_party/libmpeg2/libmpeg2','third_party/liba52/include','third_party/pl_mpeg','third_party/libdvdread/src','third_party/libdvdnav/src']:flags+=['-I'+str(root/path)]
for lib,version,keys in [('dvdread',[7,1,1],['DVDREAD_VERSION_MAJOR','DVDREAD_VERSION_MINOR','DVDREAD_VERSION_MICRO']),('dvdnav',[7,0,0],['DVDNAV_MAJOR','DVDNAV_MINOR','DVDNAV_SUB'])]:
    target=out/'include'/lib;target.mkdir(parents=True,exist_ok=True)
    text=(root/f'third_party/lib{lib}/src/{lib}/version.h.in').read_text()
    for key,value in zip(keys,version):text=text.replace('@'+key+'@',str(value))
    (target/'version.h').write_text(text)
sources=[root/'media' /p for p in ['start.c','player.c','runtime.c','dvd_reader.c']]+[root/'doom/libc.c']
sources += [root/'third_party/libmpeg2/libmpeg2'/p for p in ['alloc.c','header.c','decode.c','slice.c','motion_comp.c','idct.c','cpu_accel.c','cpu_state.c']]
sources += [root/'third_party/liba52/liba52'/p for p in ['bitstream.c','imdct.c','bit_allocate.c','parse.c','downmix.c']]
sources += [root/'third_party/libdvdread/src'/p for p in ['ifo_read.c','nav_read.c','bitreader.c','logger.c']]
sources += [root/'third_party/libdvdnav/src'/p for p in ['dvdnav.c','highlight.c','logger.c','navigation.c','read_cache.c','searching.c','settings.c','vm/decoder.c','vm/getset.c','vm/play.c','vm/vm.c','vm/vmcmd.c','vm/vmget.c']]
objects=[]
for i,p in enumerate(sources):
    obj=out/(str(i)+'-'+p.stem+'.o');objects.append(str(obj));extra=(['-I'+str(root/'third_party/libdvdnav/src')] if 'libdvdnav' in str(p) else [])+(['-mno-sse','-msoft-float'] if p.name=='start.c' else [])
    print('MEDIA compile',p.name,flush=True);subprocess.run(['gcc']+extra+flags+(['-mno-sse','-msoft-float'] if p.name=='start.c' else [])+['-c',str(p),'-o',str(obj)],check=True)
elf=root/'build/apps/player.elf';raw=elf.with_suffix('.bin')
subprocess.run(['ld','-m','elf_i386','--gc-sections','-T',str(root/'apps/link.ld'),'-o',str(elf)]+objects,check=True)
subprocess.run(['objcopy','-O','binary',str(elf),str(raw)],check=True)
subprocess.run(['python3',str(root/'tools/runpack.py'),str(elf),str(raw),str(elf.with_suffix('.run'))],check=True)
