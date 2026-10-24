"""Import the supplied BDX theme, encoding images/audio for the native OS.

Run explicitly with the supplied asset directory and generated cloud PNG.
Requires Pillow and FFmpeg on the host; ordinary OS builds use checked-in output.
Archive entries are read by exact name; no theme programs are extracted or run.
"""
from pathlib import Path
import argparse,io,shutil,struct,subprocess,zipfile
from PIL import Image,ImageOps
root=Path(__file__).resolve().parents[1]
MAPPING={'TERMINAL':'MS-DOS Application','FILEMAN':'My Documents','EDITOR':'Text File','SYSINFO':'My Computer','GAME':'Program Group','SETTINGS':'Control Panel','ABOUT':'Help','HELLO':'MS-DOS Application','CALC':'Configuration','DOOM':'Program Group','NES':'Program Group','CUBE':'Program Group','PLAYER':'Sound File','IMAGE':'Image','BROWSER':'Internet Explorer','RUN':'Run','BIN':'System File','DLL':'System File','DCL':'Configuration','FOLDER':'Folder Closed','FONT':'Document','RESET':'Log Off','POWER':'Shut Down','PUZZLE':'Program Group'}

def icon_frame(image,size):
    # Two supplied .ico files are actually BMPs with a white exterior and no
    # transparency metadata. Key only border-connected white, retaining the
    # white paper inside the document outline. Real ICOs retain their masks.
    if image.format=='ICO' and (size,size) in image.ico.sizes():return image.ico.getimage((size,size)).convert('RGBA')
    im=image.convert('RGBA')
    if image.format=='BMP':
        w,h=im.size;pixels=im.load();seen=set();pending=[(x,y) for x in range(w) for y in (0,h-1)]+[(x,y) for y in range(h) for x in (0,w-1)]
        while pending:
            x,y=pending.pop()
            if not(0<=x<w and 0<=y<h) or (x,y) in seen:continue
            seen.add((x,y));r,g,b,a=pixels[x,y]
            if min(r,g,b)<250:continue
            pixels[x,y]=(r,g,b,0);pending.extend([(x-1,y),(x+1,y),(x,y-1),(x,y+1)])
    return im

def ico(image,size=16):
    im=icon_frame(image,size).resize((size,size),Image.Resampling.LANCZOS)
    pixels=list(im.getdata());rows=[pixels[y*size:(y+1)*size] for y in range(size)]
    bmp=b''.join(bytes((b,g,r,a)) for row in reversed(rows) for r,g,b,a in row)
    mask=b''.join(bytes([sum(128>>x for x in range(8) if row[byte*8+x][3]==0) if byte<size//8 else 0 for byte in range(4)]) for row in reversed(rows))
    dib=struct.pack('<IiiHHIIiiII',40,size,size*2,1,32,0,size*size*4,0,0,0,0)+bmp+mask
    return struct.pack('<HHHBBBBHHII',0,1,1,size,size,0,0,1,32,len(dib),22)+dib

def generate_icons():
    for size,folder in [(16,'icons'),(32,'icons32')]:
        dest=root/'assets'/folder;dest.mkdir(exist_ok=True)
        for name,source in MAPPING.items():
            (dest/(name+'.ICO')).write_bytes(ico(Image.open(root/'assets/branding/xp-icons'/(source+'.ico')),size))
        (dest/'START.ICO').write_bytes(ico(Image.open(root/'assets/branding/start-logo.png'),size))
def main():
    ap=argparse.ArgumentParser();ap.add_argument('source',type=Path);ap.add_argument('wallpaper',type=Path);ap.add_argument('--ffmpeg',default='ffmpeg');a=ap.parse_args()
    out=root/'assets/branding';out.mkdir(exist_ok=True);icons=root/'assets/icons'
    for source,name in [('Dark logo.png','boot-logo.png'),('White logo_transparent.png','readme-logo.png'),('startmenu_logo art.png','start-logo.png')]:shutil.copyfile(a.source/source,out/name)
    # Format conversion and size fitting retain the supplied logo design.
    ImageOps.contain(Image.open(out/'boot-logo.png').convert('RGB'),(1024,768),Image.Resampling.LANCZOS).save(root/'assets/BOOT.BMP')
    shutil.copyfile(a.wallpaper,out/'cloud-generated.png')
    ImageOps.fit(Image.open(a.wallpaper).convert('RGB'),(3840,2160),method=Image.Resampling.LANCZOS).save(root/'assets/WALL4K.BMP')
    ImageOps.fit(Image.open(a.wallpaper).convert('RGB'),(640,360),method=Image.Resampling.LANCZOS).save(root/'assets/WALLPAPR.BMP')
    original=out/'xp-icons';original.mkdir(exist_ok=True)
    with zipfile.ZipFile(a.source/'xp_pack.zip') as z:
        (out/'xp-pack-readme.txt').write_bytes(z.read('ReadMe.txt'))
        for name,source in MAPPING.items():
            blob=z.read('Windows XP/Icons/XP '+source+'.ico');(original/(source+'.ico')).write_bytes(blob)
    generate_icons()
    sound=root/'assets/sounds';sound.mkdir(exist_ok=True)
    for source,name in [('TADA (1).WAV','START'),('DING.WAV','DING'),('CHORD.WAV','ERROR'),('finntastico-old-computer-click-152513.mp3','CLICK')]:
        shutil.copyfile(a.source/source,out/source)
        subprocess.run([a.ffmpeg,'-hide_banner','-loglevel','error','-y','-i',str(a.source/source),'-ac','1','-ar','48000','-c:a','pcm_s16le',str(sound/(name+'.WAV'))],check=True)
    print('Imported BDX logos, XP icons, cloud wallpaper and four system sounds.')
if __name__=='__main__':main()
