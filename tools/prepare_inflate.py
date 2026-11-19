from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def generate():
 out=ROOT/'build/inflate';out.mkdir(parents=True,exist_ok=True)
 source=(ROOT/'vendor/public-domain-inflate/inflate.c').read_text()
 source=source.replace('#include <ultra64.h>','#include <stdint.h>\n#include <stddef.h>\ntypedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef int32_t s32;')
 assert source.count('inbuf = (s32) (inbuf + 2);')==1
 source=source.replace('inbuf = (s32) (inbuf + 2);','inbuf = inbuf + 2;')
 source=source.replace('    inflate();\n    return wp;', '    if(inflate())return 0;\n    return wp;')
 # Keep the ROM codec distinct from zlib used by the update ZIP reader.
 source=source.replace('inflate(', 'pd_rom_inflate(')
 (out/'inflate.c').write_text(source)
 (out/'inflate.h').write_text((ROOT/'vendor/public-domain-inflate/inflate.h').read_text())
 return out
if __name__=='__main__':print(generate())
