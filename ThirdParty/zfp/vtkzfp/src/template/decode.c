#include <limits.h>

static void _t2(inv_xform, Int, DIMS)(Int* p);

/* private functions ------------------------------------------------------- */

/* inverse lifting transform of 4-vector */
static void
_t1(inv_lift, Int)(Int* p, uint s)
{
  Int x, y, z, w;
  x = *p; p += s;
  y = *p; p += s;
  z = *p; p += s;
  w = *p; p += s;

  /*
  ** non-orthogonal transform
  **       ( 4  6 -4 -1) (x)
  ** 1/4 * ( 4  2  4  5) (y)
  **       ( 4 -2  4 -5) (z)
  **       ( 4 -6 -4  1) (w)
  */
  y += w >> 1; w -= y >> 1;
  y += w; w <<= 1; w -= y;
  z += x; x <<= 1; x -= z;
  y += z; z <<= 1; z -= y;
  w += x; x <<= 1; x -= w;

  p -= s; *p = w;
  p -= s; *p = z;
  p -= s; *p = y;
  p -= s; *p = x;
}

/* map two's complement signed integer to negabinary unsigned integer */
static Int
_t1(uint2int, UInt)(UInt x)
{
  return (Int)((x ^ NBMASK) - NBMASK);
}

/* reorder unsigned coefficients and convert to signed integer */
static void
_t1(inv_order, Int)(const UInt* ublock, Int* iblock, const uchar* perm, uint n)
{
  do
    iblock[*perm++] = _t1(uint2int, UInt)(*ublock++);
  while (--n);
}

/* decompress sequence of size unsigned integers */
static uint
_t1(decode_ints, UInt)(bitstream* restrict_ stream, uint maxbits, uint maxprec, UInt* restrict_ data, uint size)
{
  /* make a copy of bit stream to avoid aliasing */
  bitstream s = *stream;
  uint intprec = CHAR_BIT * (uint)sizeof(UInt);
  uint kmin = intprec > maxprec ? intprec - maxprec : 0;
  uint bits = maxbits;
  uint i, k, m, n;
  uint64 x;

  /* initialize data array to all zeros */
  for (i = 0; i < size; i++)
    data[i] = 0;

  /* decode one bit plane at a time from MSB to LSB */
  for (k = intprec, n = 0; bits && k-- > kmin;) {
    /* decode first n bits of bit plane #k */
    m = MIN(n, bits);
    bits -= m;
    x = stream_read_bits(&s, m);
    /* unary run-length decode remainder of bit plane */
    for (; n < size && bits && (bits--, stream_read_bit(&s)); x += (uint64)1 << n++)
      for (; n < size - 1 && bits && (bits--, !stream_read_bit(&s)); n++)
        ;
    /* deposit bit plane from x */
    for (i = 0; x; i++, x >>= 1)
      data[i] += (UInt)(x & 1u) << k;
  }

  *stream = s;
  return maxbits - bits;
}

/* decompress sequence of size > 64 unsigned integers */
static uint
_t1(decode_many_ints, UInt)(bitstream* restrict_ stream, uint maxbits, uint maxprec, UInt* restrict_ data, uint size)
{
  /* make a copy of bit stream to avoid aliasing */
  bitstream s = *stream;
  uint intprec = CHAR_BIT * (uint)sizeof(UInt);
  uint kmin = intprec > maxprec ? intprec - maxprec : 0;
  uint bits = maxbits;
  uint i, k, m, n;

  /* initialize data array to all zeros */
  for (i = 0; i < size; i++)
    data[i] = 0;

  /* decode one bit plane at a time from MSB to LSB */
  for (k = intprec, n = 0; bits && k-- > kmin;) {
    /* decode first n bits of bit plane #k */
    m = MIN(n, bits);
    bits -= m;
    for (i = 0; i < m; i++)
      if (stream_read_bit(&s))
        data[i] += (UInt)1 << k;
    /* unary run-length decode remainder of bit plane */
    for (; n < size && bits && (--bits, stream_read_bit(&s)); data[n] += (UInt)1 << k, n++)
      for (; n < size - 1 && bits && (--bits, !stream_read_bit(&s)); n++)
        ;
  }

  *stream = s;
  return maxbits - bits;
}

/* decode block of integers */
static uint
_t2(decode_block, Int, DIMS)(bitstream* stream, int minbits, int maxbits, int maxprec, Int* iblock)
{
  int bits;
  cache_align_(UInt ublock[BLOCK_SIZE]);
  /* decode integer coefficients */
  if (BLOCK_SIZE <= 64)
    bits = _t1(decode_ints, UInt)(stream, maxbits, maxprec, ublock, BLOCK_SIZE);
  else
    bits = _t1(decode_many_ints, UInt)(stream, maxbits, maxprec, ublock, BLOCK_SIZE);
  /* read at least minbits bits */
  if (bits < minbits) {
    stream_skip(stream, minbits - bits);
    bits = minbits;
  }
  /* reorder unsigned coefficients and convert to signed integer */
  _t1(inv_order, Int)(ublock, iblock, PERM, BLOCK_SIZE);
  /* perform decorrelating transform */
  _t2(inv_xform, Int, DIMS)(iblock);
  return bits;
}
