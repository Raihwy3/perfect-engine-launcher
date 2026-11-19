from pathlib import Path
import ctypes,subprocess,tempfile,json
ROOT=Path(__file__).resolve().parents[1]
def main():
 out=ROOT/'build/rom-verify-host.so'
 subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC',str(ROOT/'src/rom_verify.c'),'-o',str(out)],check=True)
 lib=ctypes.CDLL(str(out));lib.pd_verify_rom.argtypes=[ctypes.c_char_p];lib.pd_verify_rom.restype=ctypes.c_int
 rom=ROOT/'local/roms/pd.ntsc-final.z64';assert lib.pd_verify_rom(str(rom).encode())==1
 with tempfile.TemporaryDirectory(prefix='pd-rom-check-') as tmp:
  bad=Path(tmp)/'bad.z64';d=bytearray(rom.read_bytes());d[-1]^=1;bad.write_bytes(d);assert lib.pd_verify_rom(str(bad).encode())==0
  bad.write_bytes(d[:4096]);assert lib.pd_verify_rom(str(bad).encode())==0
  assert lib.pd_verify_rom(str(Path(tmp)/'missing').encode())==0
 report={'full_correct_sha1':'accepted','last_byte_corruption':'rejected','truncation':'rejected','missing_file':'rejected'}
 (ROOT/'build/rom-verification.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
if __name__=='__main__':main()
