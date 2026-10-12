"""Build BIOS USB/CD/DVD installer and standalone MBR disk template.
Only writes build artifacts, never physical host drives.
"""
import argparse, io, pathlib, struct, subprocess, tarfile, zlib
ROOT=pathlib.Path(__file__).resolve().parents[1]
def run(*args): subprocess.run(list(map(str,args)),check=True)
def build(data):
    out=ROOT/'build/installer';out.mkdir(parents=True,exist_ok=True)
    cfg=(ROOT/'grub/grub.cfg').read_bytes()
    # GRUB's embedded tar contains everything required to boot the installed
    # kernel, independent of USB/CD, boot partitions or drive enumeration order.
    with tarfile.open(out/'boot.tar','w',format=tarfile.USTAR_FORMAT) as tar:
        for name,blob in [('boot/grub/grub.cfg',cfg),('boot/kernel.elf',(ROOT/'build/kernel.elf').read_bytes())]:
            entry=tarfile.TarInfo(name);entry.size=len(blob);entry.mtime=0;tar.addfile(entry,io.BytesIO(blob))
    run('grub-mkimage','-O','i386-pc','-o',out/'core.img','-p','(memdisk)/boot/grub','-m',out/'boot.tar','biosdisk','part_msdos','fat','iso9660','memdisk','tar','normal','multiboot2','all_video')
    core=bytearray((out/'core.img').read_bytes());n=(len(core)+511)//512
    assert 1<n<2047,'GRUB core exceeds MBR embedding area'
    struct.pack_into('<QHH',core,512-12,2,n-1,0x820)
    mbr=bytearray(pathlib.Path('/usr/lib/grub/i386-pc/boot.img').read_bytes())
    # A memdisk-loaded Multiboot kernel has no physical boot-device tag.
    # Preserve BIOS DL before GRUB runs, in our reserved low-memory handoff.
    # The stub executes once in boot.img's BPB/scratch area; GRUB initializes
    # that scratch space later. Keep its original entry and boot code intact.
    assert mbr[0]==0xeb,'Unsupported GRUB boot.img entry'
    entry=0x7c00+2+struct.unpack_from('<b',mbr,1)[0]
    stubsrc=out/'boot-origin.asm'
    stubsrc.write_text(f'''bits 16
org 0x7c03
cli
xor ax,ax
mov ds,ax
mov dword [0x5000],0x44534442
mov [0x5004],dl
jmp 0:{entry}
''')
    run('nasm','-f','bin',stubsrc,'-o',out/'boot-origin.bin')
    stub=(out/'boot-origin.bin').read_bytes();assert len(stub)<0x5a-3
    mbr[1]=1;mbr[3:3+len(stub)]=stub
    struct.pack_into('<Q',mbr,0x5c,1);mbr[432:440]=b'BDSINST1'
    # 1 MiB embedding area, 16 MiB FAT boot/reference volume, BDSFS data.
    boot_start,boot_blocks=2048,32768;data_start=boot_start+boot_blocks
    raw=pathlib.Path(data).read_bytes();assert raw[:8]==b'BDSFS01\0' and len(raw)==64*1024*1024
    for index,(typ,start,count) in enumerate([(6,boot_start,boot_blocks),(0x7f,data_start,len(raw)//512)]):
        struct.pack_into('<B3sB3sII',mbr,446+index*16,0x80 if index==0 else 0,b'\xfe\xff\xff',typ,b'\xfe\xff\xff',start,count)
    mbr[510:512]=b'\x55\xaa'
    fat=out/'boot-fat.img';fat.write_bytes(bytes(boot_blocks*512));run('mkfs.fat','-F','16','-n','BDSBOOT',fat)
    run('mmd','-i',fat,'::/BOOT');run('mmd','-i',fat,'::/BOOT/GRUB')
    run('mcopy','-i',fat,ROOT/'build/kernel.elf','::/BOOT/KERNEL.ELF');run('mcopy','-i',fat,ROOT/'grub/grub.cfg','::/BOOT/GRUB/GRUB.CFG')
    image=bytearray((data_start+len(raw)//512)*512);image[:512]=mbr;image[512:512+len(core)]=core
    image[boot_start*512:data_start*512]=fat.read_bytes();image[data_start*512:]=raw
    struct.pack_into('<I',image,440,0)
    struct.pack_into('<I',image,440,zlib.crc32(image))
    template=out/'system.img';template.write_bytes(image)
    iso=out/'iso';(iso/'boot/grub').mkdir(parents=True,exist_ok=True)
    (iso/'boot/kernel.elf').write_bytes((ROOT/'build/kernel.elf').read_bytes())
    (iso/'system.img').write_bytes(image)
    (iso/'README.TXT').write_text('BDS-OS BIOS installer. Dedicated disk installation erases the target.\n256 MiB RAM; legacy BIOS/CSM; FAT12/16/32 and BDSFS read/write, optical read-only.\n')
    (iso/'boot/grub/grub.cfg').write_text('''set timeout=5
set default=0
set gfxmode=800x600x32
set gfxpayload=keep
insmod all_video
terminal_output console
menuentry "Install BDS-OS to a dedicated disk (ERASES target)" {
    multiboot2 /boot/kernel.elf installer origin=$cmdpath
    module2 /system.img system-image
    boot
}
menuentry "BDS-OS desktop / inspect disks without installing" { multiboot2 /boot/kernel.elf desktop origin=$cmdpath; boot; }
menuentry "BDS-OS command line / inspect disks" { multiboot2 /boot/kernel.elf shell origin=$cmdpath; boot; }
menuentry "Reboot" { reboot; }
''')
    run('grub-mkrescue','-o',ROOT/'build/bds-os-installer.iso',iso)
    print(f'Built BIOS hybrid installer: {ROOT}/build/bds-os-installer.iso')
    print(f'Standalone disk template: {template} ({len(image)//1048576} MiB); GRUB {n} sectors')
if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--data',default=ROOT/'build/seed.img');a=ap.parse_args();build(a.data)
