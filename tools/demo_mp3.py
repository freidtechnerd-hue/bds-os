"""Encode an original test melody through the host libmp3lame; not used by the OS."""
import ctypes,math,pathlib,struct,wave
ROOT=pathlib.Path(__file__).resolve().parents[1]
lib=ctypes.CDLL('libmp3lame.so.0');lib.lame_init.restype=ctypes.c_void_p
for name in ['lame_set_in_samplerate','lame_set_num_channels','lame_set_brate','lame_set_quality']:
    getattr(lib,name).argtypes=[ctypes.c_void_p,ctypes.c_int]
lib.lame_init_params.argtypes=[ctypes.c_void_p];lib.lame_close.argtypes=[ctypes.c_void_p]
lib.lame_encode_buffer.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_short),ctypes.POINTER(ctypes.c_short),ctypes.c_int,ctypes.POINTER(ctypes.c_ubyte),ctypes.c_int]
lib.lame_encode_flush.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_ubyte),ctypes.c_int]
rate=44100;notes=[261.63,329.63,392,523.25];values=[]
for hz in notes:
    for i in range(rate//2):
        envelope=min(i/1000,(rate//2-i)/1000,1);values.append(int(7000*envelope*math.sin(2*math.pi*hz*i/rate)))
samples=(ctypes.c_short*len(values))(*values);out=(ctypes.c_ubyte*(len(values)*2))();handle=lib.lame_init()
lib.lame_set_in_samplerate(handle,rate);lib.lame_set_num_channels(handle,1);lib.lame_set_brate(handle,96);lib.lame_set_quality(handle,2);assert lib.lame_init_params(handle)>=0
n=lib.lame_encode_buffer(handle,samples,samples,len(values),out,len(out));assert n>0
encoded=bytes(out[:n]);n=lib.lame_encode_flush(handle,out,len(out));assert n>=0;encoded+=bytes(out[:n]);lib.lame_close(handle)
(ROOT/'assets/DEMO.MP3').write_bytes(encoded);print('Original MP3 melody:',len(encoded),'bytes')
