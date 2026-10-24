"""Separate native long-mode edition. Never modifies the i386 or personal images."""
from pathlib import Path
import subprocess,struct,sys
ROOT=Path(__file__).resolve().parents[1];out=ROOT/'build/x64';out.mkdir(parents=True,exist_ok=True)
def run(*args):subprocess.run(list(map(str,args)),cwd=ROOT,check=True)
flags=['-m64','-mno-red-zone','-mcmodel=small','-std=gnu11','-O2','-ffreestanding','-fno-pie','-fno-stack-protector','-fno-builtin','-mno-sse','-mno-mmx','-msoft-float','-Wall','-Wextra','-Wno-unused-parameter','-Wno-misleading-indentation','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast','-Ikernel','-Iinclude','-Ithird_party/freetype/include']
run('nasm','-f','elf64','kernel64/entry.asm','-o',out/'entry.o')
objects=[out/'entry.o']
sources=[f'kernel/{n}.c' for n in 'lib hardware video settings assets appui services ttfont font_runtime fs loader shell desktop power'.split()]+[f'kernel64/{n}.c' for n in ['main','storage','platform']]
for name in sources:
    obj=out/(Path(name).stem+'.o');run('gcc',*flags,'-c',name,'-o',obj);objects.append(obj)
run('python3','tools/build_freetype.py','64')
run('ld','-m','elf_x86_64','-T','kernel/link.ld','-o',out/'kernel.elf',*objects,'build/font64/libfont.a')
run('grub-file','--is-x86-multiboot2',out/'kernel.elf')
sys.path.insert(0,str(ROOT/'tools'));from disk import Disk
image=bytearray(64*1024*1024);image[:8]=b'BDSFS01\0';struct.pack_into('<II',image,8,len(image)//512,128)
data=out/'seed.img';data.write_bytes(image);del image;disk=Disk(data)
for name in ['hello','calc','editor','sysinfo','game','puzzle']:
    obj=out/(name+'-app.o');elf=out/(name+'.elf');raw=out/(name+'.bin')
    run('gcc',*flags,'-Wno-unused-function','-ffunction-sections','-c',f'apps/{name}.c','-o',obj)
    run('ld','-m','elf_x86_64','-T','apps/link.ld','-o',elf,obj);run('objcopy','-O','binary',elf,raw)
    symbols={line.split()[2]:int(line.split()[0],16) for line in subprocess.check_output(['nm',str(elf)],text=True).splitlines() if len(line.split())==3}
    code=raw.read_bytes();icon=(ROOT/'assets/icons32'/f'{name.upper()}.ICO').read_bytes();bss=symbols['memory_end']-0x800000-len(code)
    checksum=0
    for byte in code+icon:checksum=(checksum*31+byte)&0xffffffff
    header=struct.pack('<4sHHIIIIII',b'BDSR',2,3,len(code),bss,symbols['app_main']-0x800000,len(code)+bss,checksum,len(icon))
    (out/(name+'.run')).write_bytes(header+code+icon);disk.put('/'+name.upper()+'.RUN',header+code+icon)
for folder in ['ICONS','FONTS','DOCS']:disk.put('/'+folder)
for p in (ROOT/'assets/icons32').glob('*.ICO'):disk.put('/ICONS/'+p.name,p.read_bytes())
for p in (ROOT/'assets/fonts').glob('*.PSF'):disk.put('/FONTS/'+p.name,p.read_bytes())
disk.put('/WALL4K.BMP',(ROOT/'assets/WALL4K.BMP').read_bytes())
disk.put('/BOOT.BMP',(ROOT/'assets/BOOT.BMP').read_bytes())
disk.put('/FONTS/DEJAVU.TTF',(ROOT/'third_party/dejavu/DejaVuSansMono.ttf').read_bytes())
disk.put('/DOCS/FREETYPE-LICENSE.TXT',(ROOT/'third_party/freetype/docs/FTL.TXT').read_bytes())
disk.put('/WALLPAPR.BMP',(ROOT/'assets/WALLPAPR.BMP').read_bytes())
disk.put('/NOTES.TXT',b'BDX-OS x64 native editor. F2 saves on the writable data image.\n')
disk.put('/README.TXT',b'Native x64 preview: HELLO, CALC, EDITOR, SYSINFO, GAME, PUZZLE.\nNo i386, Windows or Linux binary compatibility; no networking or audio yet.\n')
disk.put('/DOCS/BDX-ASSETS.TXT',(ROOT/'assets/branding/README.md').read_bytes())
disk.put('/DOCS/XP-THEME.TXT',(ROOT/'assets/branding/xp-pack-readme.txt').read_bytes())
disk.put('/DOCS/2048-LICENSE.TXT',(ROOT/'third_party/2048/LICENSE.txt').read_bytes())
disk.put('/DOCS/FONT-LICENSE.TXT',(ROOT/'third_party/dejavu/LICENSE').read_bytes());disk.save();disk.check()
disk.put('/DOCS/X64.TXT',(ROOT/'docs/X64.md').read_bytes());disk.save();disk.check()
iso=out/'iso/boot';(iso/'grub').mkdir(parents=True,exist_ok=True)
import shutil
shutil.copyfile(out/'kernel.elf',iso/'kernel.elf');shutil.copyfile(data,iso/'system.img')
(iso/'grub/grub.cfg').write_text('''set timeout=3
set default=0
insmod all_video
set gfxmode=1024x768x32,800x600x32,auto
set gfxpayload=keep
menuentry "BDX-OS x64 preview - native desktop" {
 multiboot2 /boot/kernel.elf
 module2 /boot/system.img
 boot
}
''')
run('grub-mkrescue','-o',out/'bds-os-x64.iso',out/'iso')
print('Built true x86-64 kernel, six native x64 apps, live ISO and separate writable data seed.',flush=True)
