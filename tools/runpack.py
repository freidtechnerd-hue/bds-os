import pathlib,struct,subprocess,sys
elf,raw,out=sys.argv[1:4]
root=pathlib.Path(__file__).resolve().parents[1]
icon=pathlib.Path(sys.argv[4]) if len(sys.argv)>4 else root/"assets/icons"/(pathlib.Path(out).stem.upper()+".ICO")
if not icon.exists():icon=root/"assets/icons/RUN.ICO"
icon=icon.read_bytes()
assert 22<=len(icon)<=2048 and icon[:8]==bytes([0,0,1,0,1,0,16,16]), "Expected a 16x16 ICO"
symbols={}
for line in subprocess.check_output(['nm',elf],text=True).splitlines():
    fields=line.split()
    if len(fields)==3:symbols[fields[2]]=int(fields[0],16)
code=pathlib.Path(raw).read_bytes();bss=symbols['memory_end']-0x800000-len(code);entry=symbols['app_main']-0x800000
checksum=0
for c in code+icon:checksum=(checksum*31+c)&0xffffffff
header=struct.pack('<4sHHIIIIII',b'BDSR',2,2,len(code),bss,entry,len(code)+bss,checksum,len(icon))
pathlib.Path(out).write_bytes(header+code+icon)
print(f'{out}: {len(code)} code/data bytes, {bss} zero bytes, entry {entry}')
