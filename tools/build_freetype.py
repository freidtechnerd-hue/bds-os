"""Integer-only FreeType TrueType/gray rasterizer for the two native kernels."""
from pathlib import Path
import subprocess,sys
root=Path(__file__).resolve().parents[1]
bits=sys.argv[1] if len(sys.argv)>1 else '32'
out=root/'build'/('font'+bits);out.mkdir(exist_ok=True)
flags=['-m'+bits,'-O2','-ffreestanding','-fno-builtin','-fno-pie','-fno-stack-protector','-mno-sse','-mno-mmx','-msoft-float','-U_FORTIFY_SOURCE','-D_FORTIFY_SOURCE=0','-DFT2_BUILD_LIBRARY','-DFT_CONFIG_MODULES_H="ftmodule.h"','-DFT_CONFIG_OPTIONS_H="ftoption.h"','-Ikernel/fontconfig','-Ithird_party/freetype/include']
flags+=['-include','kernel/fontconfig/ftstdlib.h']
if bits=='64':flags+=['-mno-red-zone','-mcmodel=small']
objects=[]
sources=['base/ftbase','base/ftinit','base/ftdebug','base/ftbitmap','base/ftmm','gzip/ftgzip','sfnt/sfnt','truetype/truetype','smooth/smooth']
# Preserve each edition's established link order in fresh and incremental
# builds. ar r replaces members in place, which otherwise retains stale order.
if bits=='32':sources=['base/ftbase','base/ftinit','base/ftdebug','sfnt/sfnt','truetype/truetype','smooth/smooth','base/ftbitmap','base/ftmm','gzip/ftgzip']
for name in sources:
    obj=out/(name.replace('/','-')+'.o');subprocess.run(['gcc',*flags,'-c','third_party/freetype/src/'+name+'.c','-o',str(obj)],cwd=root,check=True);objects.append(str(obj))
archive=out/'libfont.a'
archive.unlink(missing_ok=True)
subprocess.run(['ar','rcs',str(archive),*objects],check=True)
