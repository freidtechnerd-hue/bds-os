"""Freestanding BearSSL archive. Host sources and public roots are pinned in-tree."""
import pathlib,subprocess,concurrent.futures
root=pathlib.Path(__file__).resolve().parents[1];out=root/'build/tls';out.mkdir(parents=True,exist_ok=True)
inc=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m32','-Os','-std=gnu11','-ffreestanding','-fno-builtin','-fno-pie','-fno-stack-protector','-mno-sse','-mno-mmx','-msoft-float','-ffunction-sections','-fdata-sections','-nostdinc','-isystem',inc,'-I'+str(root/'doom/include'),'-I'+str(root/'third_party/bearssl/inc'),'-I'+str(root/'third_party/bearssl/src')]
flags += ['-D'+x+'=0' for x in ['BR_USE_URANDOM','BR_USE_GETENTROPY','BR_USE_WIN32_RAND','BR_USE_UNIX_TIME','BR_USE_WIN32_TIME','BR_RDRAND','BR_AES_X86NI','BR_SSE2','BR_POWER8','BR_INT128','BR_UMUL128']]
sources=sorted((root/'third_party/bearssl/src').rglob('*.c'))+[root/'kernel/tls.c']
def compile(p):
    obj=out/(p.stem+'.o')
    subprocess.run(['gcc']+flags+['-c',str(p),'-o',str(obj)],check=True)
    return str(obj)
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:objects=list(pool.map(compile,sources))
subprocess.run(['ar','rcs',str(out/'libtls.a')]+objects,check=True)
print('TLS archive compiled:',len(objects),'objects')
