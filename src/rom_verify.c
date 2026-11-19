/* Streaming verifier adapted from the custom Eyes of Gold converter. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint32_t be32(const unsigned char *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
struct Sha {
  uint32_t h[5];
  uint64_t bytes;
  unsigned used;
  unsigned char block[64];
};
static uint32_t rol(uint32_t v, unsigned n) { return v << n | v >> (32 - n); }
static void sha_block(struct Sha *s) {
  uint32_t w[80], a = s->h[0], b = s->h[1], c = s->h[2], d = s->h[3],
                  e = s->h[4];
  for (unsigned i = 0; i < 80; i++)
    w[i] = i < 16 ? be32(s->block + 4 * i)
                  : rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
  for (unsigned i = 0; i < 80; i++) {
    uint32_t f = i < 20   ? ((b & c) | (~b & d))
                 : i < 40 ? (b ^ c ^ d)
                 : i < 60 ? ((b & c) | (b & d) | (c & d))
                          : (b ^ c ^ d);
    uint32_t k = i < 20   ? 0x5a827999u
                 : i < 40 ? 0x6ed9eba1u
                 : i < 60 ? 0x8f1bbcdcu
                          : 0xca62c1d6u;
    uint32_t t = rol(a, 5) + f + e + k + w[i];
    e = d;
    d = c;
    c = rol(b, 30);
    b = a;
    a = t;
  }
  s->h[0] += a;
  s->h[1] += b;
  s->h[2] += c;
  s->h[3] += d;
  s->h[4] += e;
}
static void sha_feed(struct Sha *s, const void *data, size_t n) {
  const unsigned char *p = data;
  s->bytes += n;
  while (n) {
    unsigned k = 64 - s->used;
    if (k > n)
      k = (unsigned)n;
    memcpy(s->block + s->used, p, k);
    s->used += k;
    p += k;
    n -= k;
    if (s->used == 64) {
      sha_block(s);
      s->used = 0;
    }
  }
}
static int sha_finish(struct Sha *s) {
  uint64_t bits = s->bytes * 8;
  unsigned char pad[72] = {128};
  unsigned n = s->used < 56 ? 56 - s->used : 120 - s->used;
  sha_feed(s, pad, n);
  for (unsigned i = 0; i < 8; i++)
    pad[i] = (unsigned char)(bits >> (56 - 8 * i));
  sha_feed(s, pad, 8);
  const uint32_t expected[] = {0xaf8788ac, 0x4d1a5726, 0x0eae9c53, 0xffe851fc,
                               0xf2a3319b};
  return !memcmp(s->h, expected, sizeof expected);
}

extern void pd_boot_rom_progress(unsigned,unsigned) __attribute__((weak));
int pd_verify_rom(const char *path) {
 FILE *f=fopen(path,"rb");if(!f)return 0;
 struct Sha sha={{0x67452301,0xefcdab89,0x98badcfe,0x10325476,0xc3d2e1f0},0,0,{0}};
 unsigned char bytes[32768];size_t n;
 while((n=fread(bytes,1,sizeof bytes,f))){sha_feed(&sha,bytes,n);if(pd_boot_rom_progress && !(sha.bytes%1048576))pd_boot_rom_progress((unsigned)sha.bytes,33554432);}
 int ok=!ferror(f) && sha.bytes==33554432;fclose(f);
 return ok && sha_finish(&sha);
}
