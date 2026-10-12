"""Compile upstream doomgeneric with our freestanding libc and BDS platform."""
import pathlib,subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
src=ROOT/'third_party/doomgeneric/doomgeneric';out=ROOT/'build/doom';out.mkdir(parents=True,exist_ok=True)
headers='stdio stdlib stdint inttypes string strings limits ctype errno assert math unistd fcntl'.split()
for h in headers:(ROOT/f'doom/include/{h}.h').write_text('#include "libc.h"\n')
(ROOT/'doom/include/sys').mkdir(exist_ok=True)
for h in ['types','stat']:(ROOT/f'doom/include/sys/{h}.h').write_text('#include "libc.h"\n')
names='dummy am_map doomdef doomstat dstrings d_event d_items d_iwad d_loop d_main d_mode d_net f_finale f_wipe g_game hu_lib hu_stuff info i_cdmus i_endoom i_joystick i_scale i_sound i_system i_timer memio m_argv m_bbox m_cheat m_config m_controls m_fixed m_menu m_misc m_random p_ceilng p_doors p_enemy p_floor p_inter p_lights p_map p_maputl p_mobj p_plats p_pspr p_saveg p_setup p_sight p_spec p_switch p_telept p_tick p_user r_bsp r_data r_draw r_main r_plane r_segs r_sky r_things sha1 sounds statdump st_lib st_stuff s_sound tables v_video wi_stuff w_checksum w_file w_main w_wad z_zone w_file_stdc i_input i_video doomgeneric'.split()
inc=subprocess.check_output(['gcc','-print-file-name=include'],text=True).strip()
flags=['-m32','-Os','-ffreestanding','-fno-builtin','-fno-pie','-fno-stack-protector','-mno-sse','-mno-mmx','-mfpmath=387','-ffunction-sections','-fdata-sections','-nostdinc','-isystem',inc,'-I'+str(ROOT/'doom/include'),'-I'+str(ROOT/'include'),'-I'+str(src),'-DDOOMGENERIC_RESX=320','-DDOOMGENERIC_RESY=200','-DNORMALUNIX','-D_DEFAULT_SOURCE','-Wno-pointer-sign']
objects=[]
header_time=max(p.stat().st_mtime for p in list(src.glob('*.h'))+[ROOT/'doom/include/libc.h',ROOT/'include/bds.h'])
for p in [src/(n+'.c') for n in names if n!='i_sound']+[ROOT/'doom/libc.c',ROOT/'doom/platform.c',ROOT/'doom/sound.c']:
    obj=out/(p.stem+'.o');objects.append(str(obj))
    if not obj.exists() or obj.stat().st_mtime<max(p.stat().st_mtime,header_time):
        print('DOOM compile',p.name,flush=True);subprocess.run(['gcc']+flags+['-c',str(p),'-o',str(obj)],check=True)
elf=ROOT/'build/apps/doom.elf';raw=elf.with_suffix('.bin');run=elf.with_suffix('.run')
subprocess.run(['ld','-m','elf_i386','--gc-sections','-T',str(ROOT/'apps/link.ld'),'-o',str(elf)]+objects,check=True)
subprocess.run(['objcopy','-O','binary',str(elf),str(raw)],check=True)
subprocess.run(['python3',str(ROOT/'tools/runpack.py'),str(elf),str(raw),str(run)],check=True)
