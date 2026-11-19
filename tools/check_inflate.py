#!/usr/bin/env python3
"""Compare the actual target codec to zlib for the ROM's compressed resources."""
import ctypes,hashlib,json,struct,subprocess,zlib
from pathlib import Path
from prepare_inflate import generate
ROOT=Path(__file__).resolve().parents[1]
def main():
 out=generate();lib=out/'inflate-host.so'
 subprocess.run(['gcc','-shared','-fPIC','-O2','-w','-I'+str(out),str(out/'inflate.c'),str(ROOT/'src/rzip_3ds.c'),'-o',str(lib)],check=True)
 codec=ctypes.CDLL(str(lib));codec.rzipInflate.argtypes=[ctypes.c_void_p]*3;codec.rzipInflate.restype=ctypes.c_int32
 rom=(ROOT/'local/roms/pd.ntsc-final.z64').read_bytes();assert hashlib.md5(rom).hexdigest()=='e03b088b6ac9e0080440efed07c1e40f'
 def check(blob,label):
  kind=blob[:2];skip=5 if kind==b'\x11\x73' else 2
  if kind not in (b'\x11\x72',b'\x11\x73'):return False
  expected=zlib.decompress(blob[skip:],-15)
  if skip==5:assert len(expected)==int.from_bytes(blob[2:5],'big'),label
  source=ctypes.create_string_buffer(blob+b'\0'*32);dest=ctypes.create_string_buffer(len(expected)+64)
  assert codec.rzipInflate(source,dest,None)==len(expected),label
  assert dest.raw[:len(expected)]==expected,label
  assert dest.raw[len(expected):]==b'\0'*64,label
  return True
 assert check(rom[0x39850:],'data segment')
 data=zlib.decompress(rom[0x39855:],-15);offsets=[]
 for i in range(1,2048):
  at=struct.unpack_from('>I',data,0x28080+i*4)[0]
  if not at:break
  offsets.append(at)
 count=0
 for i,(at,end) in enumerate(zip(offsets,offsets[1:]),1):
  if check(rom[at:end],f'file {i}'):count+=1
 report={'codec':'public-domain inflate / Rare framing','data_segment':'byte-identical','compressed_resources_checked':count,'all_outputs':'byte-identical to Python zlib','output_canaries':'intact'}
 (out/'validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
