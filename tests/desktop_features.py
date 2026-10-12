"""App host window/fullscreen, sidecar icon, fonts, resources and NES browser."""
import json,pathlib,shutil,subprocess,sys,time
from interactive import VM,ROOT,LOG,pixel
sys.path.insert(0,str(ROOT/'tools'));from disk import Disk
from nes import cartridge
def compile_fixture():
    out=ROOT/'build/features';out.mkdir(exist_ok=True)
    subprocess.run(['gcc','-m32','-Os','-ffreestanding','-fno-builtin','-fno-pie','-fno-stack-protector','-I'+str(ROOT/'include'),'-c',str(ROOT/'tests/features_app.c'),'-o',str(out/'app.o')],check=True)
    subprocess.run(['ld','-m','elf_i386','-T',str(ROOT/'apps/link.ld'),'-o',str(out/'app.elf'),str(out/'app.o')],check=True)
    subprocess.run(['objcopy','-O','binary',str(out/'app.elf'),str(out/'app.bin')],check=True)
    subprocess.run(['python3',str(ROOT/'tools/runpack.py'),str(out/'app.elf'),str(out/'app.bin'),str(out/'future.run')],check=True);return out/'future.run'
def main():
    disk=ROOT/'build/features-test.img';shutil.copyfile(ROOT/'build/seed.img',disk);d=Disk(disk)
    d.put('/FUTURE.RUN',compile_fixture().read_bytes());d.put('/FUTURE.ICO',(ROOT/'assets/icons/PLAYER.ICO').read_bytes());d.put('/NES/TEST.NES',cartridge());d.save()
    v=VM('desktop-features',disk)
    try:
        v.wait('BOOT BMP splash ready');v.capture('boot-custom-bmp');v.wait('DESKTOP ready');v.capture('desktop-icons')
        assert pixel(LOG/'desktop-icons.ppm',400,300)==bytes.fromhex('000000')
        assert pixel(LOG/'desktop-icons.ppm',401,302)==bytes.fromhex('ffffff')
        v.command('RUN /FUTURE.RUN','FEATURES: resources and custom font passed');v.wait('APP window frame ready');v.capture('future-windowed');windowed=(LOG/'future-windowed.ppm').read_bytes()
        v.key('ctrl-f');v.wait('APP fullscreen enabled');time.sleep(.3);v.capture('future-fullscreen');assert windowed!=(LOG/'future-fullscreen.ppm').read_bytes()
        assert pixel(LOG/'future-fullscreen.ppm',400,300)==bytes.fromhex('000000')
        assert pixel(LOG/'future-fullscreen.ppm',401,302)==bytes.fromhex('ffffff')
        start=len(v.log);v.key('ctrl-f');v.wait('APP windowed enabled',start=start);time.sleep(.3);v.capture('future-windowed-return');assert pixel(LOG/'future-windowed-return.ppm',100,45)==bytes.fromhex('36594b')
        start=len(v.log);v.key('esc');v.wait('RUN returned: 0',start=start)
        v.command('FONT DEFAULT','Built-in font restored')
        v.command('NES','NES library ready');v.wait('FILE MANAGER OPEN: .NES');v.capture('nes-file-dialog');v.key('ret');v.wait('NES native emulation started');v.wait('NES frame 120');v.capture('nes-new-window')
        # File -> Open runs the same browser while a cartridge is active.
        v.mouse(-296,-230);time.sleep(.4);v.mouse(down=True);time.sleep(.4);v.mouse(down=False);time.sleep(.4)
        v.capture('nes-file-menu');v.mouse(10,20);time.sleep(.4);v.mouse(down=True);time.sleep(.4);v.mouse(down=False)
        start=len(v.log);v.wait('FILE MANAGER OPEN: .NES',start=start);v.key('esc');time.sleep(.2)
        start=len(v.log);v.key('esc');v.wait('NES exit to BDS-OS',start=start);v.wait('RUN returned: 0',start=start)
        v.command('CHECK','PASS: filesystem integrity')
    finally:v.close()
    (LOG/'desktop-features-results.json').write_text(json.dumps({'passed':['supplied cursor over desktop and fullscreen apps','original BMP boot splash','desktop and future RUN icons','BIN DLL DCL resources','PSF custom font and invalid font rejection','windowed default and Ctrl+F round trip','NES file manager startup and File menu browser','clean return and filesystem integrity']},indent=2))
    print('PASS desktop icons, app windows/fullscreen, resources, custom fonts and NES browser',flush=True)
if __name__=='__main__':main()
