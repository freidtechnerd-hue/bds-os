"""FAT12 1.44MB builder: 65 reserved sectors, two FATs, root, cluster data."""
import pathlib, struct, sys
ROOT = pathlib.Path(__file__).resolve().parents[1]
def create(path):
    img = bytearray(1474560)
    boot = (ROOT/'build/boot16.bin').read_bytes()
    kernel = (ROOT/'build/kernel16.bin').read_bytes()
    assert len(boot) == 512 and len(kernel) <= 32768
    img[:512] = boot
    img[512:512+len(kernel)] = kernel
    fat = bytearray(4608); fat[:3] = b'\xf0\xff\xff'
    def setfat(n,v):
        p = n+n//2
        old = int.from_bytes(fat[p:p+2], 'little')
        old = (old & 15) | (v<<4) if n&1 else (old & 0xf000) | v
        fat[p:p+2] = old.to_bytes(2,'little')
    cluster = 2
    files = [(b'README  TXT', b'BDS-OS 8086: real BIOS boot, FAT12, native programs.\r\n'),
             (b'HELLO   RUN',(ROOT/'build/hello16.run').read_bytes())]
    for i,(name,data) in enumerate(files):
        entry = 83*512+i*32
        img[entry:entry+11] = name
        img[entry+11] = 0x20
        struct.pack_into('<H',img,entry+26,cluster)
        struct.pack_into('<I',img,entry+28,len(data))
        count = max(1,(len(data)+511)//512)
        img[(cluster+95)*512:(cluster+95)*512+len(data)] = data
        for j in range(count): setfat(cluster+j, 0xfff if j==count-1 else cluster+j+1)
        cluster += count
    img[65*512:74*512] = fat; img[74*512:83*512] = fat
    pathlib.Path(path).write_bytes(img)
    print(f'FAT12: {path}, kernel {len(kernel)} bytes, {len(files)} files')
if __name__ == '__main__':
    if sys.argv[1] == 'create': create(sys.argv[2])
