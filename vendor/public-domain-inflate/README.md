Inflater from the pinned GoldenEye reference, src/inflate/inflate.c and .h.
The source identifies its gzip inflate core as public domain by Mark Adler.
Original files are preserved. tools/prepare_inflate.py generates the portable
copy with explicit standard integer types and a pointer-width fix. No game
art, audio, model data or other ROM payload is present in this codec.
