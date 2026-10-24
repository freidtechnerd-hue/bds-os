"""Original 16x16 Windows ICO files, PSF1 fonts and companion data examples."""
import pathlib,re,struct,sys,shutil
ROOT=pathlib.Path(__file__).resolve().parents[1]
def generate():
    out=ROOT/'assets';(out/'icons').mkdir(parents=True,exist_ok=True);(out/'fonts').mkdir(exist_ok=True)
    names=['TERMINAL','FILEMAN','EDITOR','SYSINFO','GAME','SETTINGS','ABOUT','HELLO','CALC','DOOM','NES','CUBE','PLAYER','IMAGE','BROWSER','RUN','BIN','DLL','DCL','FOLDER','FONT','START','RESET','POWER']
    for name in names:
        pixels=[[0 for x in range(16)] for y in range(16)]
        def box(x,y,w,h,c):
            for yy in range(y,y+h):
                for xx in range(x,x+w):pixels[yy][xx]=c
        box(2,2,12,12,0xff172c24);box(3,3,10,10,0xffd5dfcb)
        if name in ['FOLDER','FILEMAN']:box(1,5,14,9,0xffd8b35b);box(2,3,6,3,0xffd8b35b);box(2,7,12,6,0xffedd297)
        elif name in ['TERMINAL','HELLO']:box(3,4,10,7,0xff18291e);box(4,5,2,1,0xff8ed7a2);box(5,6,2,1,0xff8ed7a2);box(4,7,2,1,0xff8ed7a2);box(8,9,3,1,0xff8ed7a2)
        elif name=='GAME':box(1,6,14,6,0xff798976);box(3,6,1,5,0xff26362b);box(1,8,5,1,0xff26362b);box(10,7,2,2,0xffc96143);box(12,9,2,2,0xffdfb74e)
        elif name=='NES':box(3,2,10,12,0xff929a8c);box(5,3,6,6,0xffe0d5b8);box(6,4,4,2,0xffbe5943);box(5,11,6,3,0xff26362b)
        elif name=='CUBE':box(2,3,12,11,0xff795536);box(2,3,12,4,0xff68a447);box(3,7,2,2,0xff9a784e);box(8,10,3,2,0xff553b29);box(11,7,2,3,0xffb08b59)
        elif name=='DOOM':box(3,3,10,10,0xffb44e38);box(4,5,8,4,0xffd5dfcb);box(5,6,2,2,0xff26362b);box(9,6,2,2,0xff26362b);box(6,10,4,2,0xff26362b)
        elif name=='PLAYER':box(7,4,1,7,0xff3e7153);box(8,4,4,1,0xff3e7153);box(11,5,1,5,0xff3e7153);box(4,10,4,2,0xff3e7153);box(9,9,3,2,0xff3e7153)
        elif name=='IMAGE':box(3,3,10,10,0xff73b7dc);box(9,4,2,2,0xffffda7b);box(4,9,4,3,0xff50835f);box(7,8,5,4,0xff355b42)
        elif name=='BROWSER':box(3,3,10,10,0xff518fa5);box(4,7,8,2,0xffe0eaca);box(7,4,2,8,0xffe0eaca);box(5,4,6,1,0xffe0eaca);box(5,11,6,1,0xffe0eaca)
        elif name=='START':box(4,4,3,3,0xff679ce0);box(9,4,3,3,0xffe8bc61);box(4,9,3,3,0xff6fbc82);box(9,9,3,3,0xffd57765)
        elif name=='POWER':box(4,5,2,7,0xffb25145);box(10,5,2,7,0xffb25145);box(5,11,6,2,0xffb25145);box(7,2,2,7,0xffb25145)
        elif name=='RESET':box(4,4,8,2,0xff4a7d9e);box(10,5,2,6,0xff4a7d9e);box(4,10,7,2,0xff4a7d9e);box(3,8,3,5,0xff4a7d9e)
        elif name=='CALC':box(4,4,8,3,0xff3d6651);[box(x,y,2,2,0xff596858) for y in (8,11) for x in (4,7,10)]
        elif name in ['EDITOR','DCL']:box(5,4,6,1,0xff4f6954);box(5,7,6,1,0xff4f6954);box(5,10,4,1,0xff4f6954);box(11,5,2,7,0xffd1a44e)
        elif name in ['DLL','BIN','RUN']:box(4,4,8,8,0xff436b55);box(6,6,4,4,0xffc7d4b9);[box(x,1,1,2,0xff839078) for x in (4,7,10)];[box(x,13,1,2,0xff839078) for x in (4,7,10)]
        elif name=='FONT':box(7,4,2,2,0xff436b55);box(5,6,2,6,0xff436b55);box(9,6,2,6,0xff436b55);box(6,9,4,1,0xff436b55)
        elif name=='SETTINGS':box(4,4,8,8,0xff667f66);box(6,6,4,4,0xffd5dfcb);box(7,1,2,3,0xff667f66);box(7,12,2,3,0xff667f66);box(1,7,3,2,0xff667f66);box(12,7,3,2,0xff667f66)
        elif name=='ABOUT':box(4,4,8,8,0xff689b89);box(7,5,2,4,0xffd5dfcb);box(7,11,2,1,0xffd5dfcb)
        else:box(7,4,2,2,0xff436b55);box(7,7,2,5,0xff436b55);box(5,11,6,1,0xff436b55)
        dib=struct.pack('<IiiHHIIiiII',40,16,32,1,32,0,1024,0,0,0,0)
        bitmap=b''.join(struct.pack('<I',c) for row in reversed(pixels) for c in row)
        mask=b''.join(bytes([sum((128>>x) for x in range(8) if not row[x]),sum((128>>x) for x in range(8) if not row[8+x]),0,0]) for row in reversed(pixels))
        icon=struct.pack('<HHH',0,1,1)+struct.pack('<BBBBHHII',16,16,0,0,1,32,len(dib+bitmap+mask),22)+dib+bitmap+mask
        iconpath=out/'icons'/f'{name}.ICO'
        if not iconpath.exists():iconpath.write_bytes(icon)
    font=(ROOT/'kernel/font8x8.h').read_text();rows=re.findall(r'\{([^{}]+)\}',font)
    glyphs=[]
    for row in rows:
        values=re.findall(r'0x([0-9a-fA-F]{2})',row)
        if len(values)==8:glyphs.append(bytes(int(f'{int(x,16):08b}'[::-1],2) for x in values))
    assert len(glyphs)==128
    body=b''.join(glyphs)+bytes(128*8);(out/'fonts/DEFAULT.PSF').write_bytes(b'\x36\x04\x00\x08'+body)
    bold=bytes(x|(x>>1) for x in body);(out/'fonts/BOLD.PSF').write_bytes(b'\x36\x04\x00\x08'+bold)
    # Original short melody; no external copyrighted soundtrack.
    track=bytearray(b'\x00\xff\x51\x03\x07\xa1\x20\x00\xc0\x50')
    for note in [60,64,67,72,71,67,64,60]:track+=bytes([0,0x90,note,96,96,0x80,note,0])
    track+=b'\x00\xff\x2f\x00';(out/'DEMO.MID').write_bytes(b'MThd'+struct.pack('>IHHH',6,0,1,96)+b'MTrk'+struct.pack('>I',len(track))+track)
def populate(disk):
    out=ROOT/'assets';disk.put('/ICONS');disk.put('/FONTS');disk.put('/DATA');disk.put('/MUSIC');disk.put('/DOCS')
    for p in (out/'icons').glob('*.ICO'):disk.put('/ICONS/'+p.name,p.read_bytes())
    for p in (out/'fonts').glob('*.PSF'):disk.put('/FONTS/'+p.name,p.read_bytes())
    for app in (ROOT/'build/apps').glob('*.run'):
        disk.put('/'+app.name.upper(),app.read_bytes());icon=out/'icons'/f'{app.stem.upper()}.ICO'
        if icon.exists():disk.put('/'+icon.name,icon.read_bytes())
    if (out/'BOOT.BMP').exists():disk.put('/BOOT.BMP',(out/'BOOT.BMP').read_bytes())
    for source,name in [('WALLPAPR.BMP','/WALLPAPR.BMP'),('wallpaper-source.png','/CLOUDS.PNG'),('HOME.HTM','/HOME.HTM')]:disk.put(name,(out/source).read_bytes())
    for name in ['DEMO.MID','DEMO.MP3']:
        if (out/name).exists():disk.put('/MUSIC/'+name,(out/name).read_bytes())
    disk.put('/DATA/EXAMPLE.BIN',b'BDS binary companion data\0\x01\x02\x03')
    disk.put('/DATA/EXAMPLE.DLL',b'BDS companion data; not a Windows executable DLL.\n')
    disk.put('/DATA/EXAMPLE.DCL',b'caption=BDS example\nicon=/ICONS/RUN.ICO\n')
    for source,name in [('third_party/dejavu/LICENSE','DEJAVU-LICENSE.TXT'),('docs/HTTPS.md','HTTPS.TXT'),('third_party/bearssl/LICENSE.txt','BEARSSL-LICENSE.TXT'),('third_party/ca-certificates/LICENSE','CA-LICENSE.TXT'),('third_party/libdvdnav/COPYING','DVDNAV-LICENSE.TXT'),('third_party/libdvdread/COPYING','DVDREAD-LICENSE.TXT'),('docs/MEDIA.md','MEDIA.TXT'),('third_party/libmpeg2/COPYING','LIBMPEG2-LICENSE.TXT'),('third_party/liba52/COPYING','LIBA52-LICENSE.TXT'),('third_party/pl_mpeg/LICENSE','PLMPEG-LICENSE.TXT'),('docs/DESKTOP-UPGRADE.md','DESKTOP-UPGRADE.TXT'),('third_party/elk/LICENSE','ELK-NOTICE.TXT'),('licenses/AGPL-3.0.txt','AGPL-3.0.TXT'),('docs/DESKTOP-AUDIO.md','DESKTOP-AUDIO.TXT'),('docs/NETWORK-DESKTOP.md','NETWORK-DESKTOP.TXT'),('licenses/NOTICE.md','BDS-NOTICES.TXT'),('third_party/minimp3/LICENSE','MINIMP3-LICENSE.TXT'),('third_party/lwip/COPYING','LWIP-LICENSE.TXT'),('third_party/gumbo/COPYING','GUMBO-LICENSE.TXT'),('third_party/lodepng/LICENSE','LODEPNG-LICENSE.TXT')]:
        disk.put('/DOCS/'+name,(ROOT/source).read_bytes())
    for source,name in [('FONTS.md','FONTS.TXT'),('COMPATIBILITY.md','COMPAT.TXT'),('GAMING.md','GAMING.TXT'),('MODERN-PLATFORM.md','MODERN.TXT'),('GETTING-STARTED.md','INSTALL-GUIDE.TXT'),('CREDITS.md','PROJECT-CREDITS.TXT'),('CHANGELOG.md','CHANGELOG.TXT')]:
        disk.put('/DOCS/'+name,(ROOT/'docs'/source).read_bytes())
    disk.put('/DOCS/BDX-ASSETS.TXT',(ROOT/'assets/branding/README.md').read_bytes())
    disk.put('/DOCS/XP-THEME.TXT',(ROOT/'assets/branding/xp-pack-readme.txt').read_bytes())
    disk.put('/SOUNDS')
    for p in (out/'sounds').glob('*.WAV'):disk.put('/SOUNDS/'+p.name,p.read_bytes())
    disk.put('/FONTS/DEJAVU.TTF',(ROOT/'third_party/dejavu/DejaVuSansMono.ttf').read_bytes())
    disk.put('/DOCS/FREETYPE-LICENSE.TXT',(ROOT/'third_party/freetype/docs/FTL.TXT').read_bytes())
    disk.put('/CLASSIC');disk.put('/CLASSIC/texpacks');disk.put('/CLASSIC/maps')
    disk.put('/CLASSIC/texpacks/classicube.zip',(ROOT/'third_party/classicube/misc/cc_textures.zip').read_bytes())
    disk.put('/DOCS/2048-LICENSE.TXT',(ROOT/'third_party/2048/LICENSE.txt').read_bytes())
    disk.put('/DOCS/CLASSICUBE.TXT',(ROOT/'docs/CLASSICUBE.md').read_bytes())
    disk.put('/DOCS/CLASSICUBE-LICENSE.TXT',(ROOT/'third_party/classicube/license.txt').read_bytes())
    if disk.find('/CLASSIC/options.txt')<0:
        disk.put('/CLASSIC/options.txt',b'defaulttexpack=classicube.zip\nviewdist=32\nfpslimit=Limit60FPS\nchat-logging=false\nmusicvolume=0\nsoundsvolume=0\n')
    disk.save();disk.check()
if __name__=='__main__':
    generate()
    if len(sys.argv)>1:
        from disk import Disk
        image=pathlib.Path(sys.argv[1]);backup=image.with_suffix('.before-desktop.img')
        if not backup.exists():shutil.copyfile(image,backup)
        populate(Disk(image))
