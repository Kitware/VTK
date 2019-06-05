static void _t2(rev_fwd_xform, Int, DIMS)(Int* p);

/* private functions ------------------------------------------------------- */

/* reversible forward lifting transform of 4-vector */
static void
_t1(rev_fwd_lift, Int)(Int* p, uint s)
{
  Int x, y, z, w;
  x = *p; p += s;
  y = *p; p += s;
  z = *p; p += s;
  w = *p; p += s;

  /*
  ** high-order Lorenzo transform
  ** ( 1  0  0  0) (x)
  ** (-1  1  0  0) (y)
  ** ( 1 -2  1  0) (z)
  ** (-1  3 -3  1) (w)
  */
  w -= z; z -= y; y -= x;
  w -= z; z -= y;
  w -= z;

  p -= s; *p = w;
  p -= s; *p = z;
  p -= s; *p = y;
  p -= s; *p = x;
}

/* return precision required to encode block reversibly */
static uint
_t1(rev_precision, UInt)(const UInt* block, uint n)
{
  uint p = 0;
  uint s;
  /* compute bitwise OR of all values */
  UInt m = 0;
  while (n--)
    m |= *block++;
  /* count trailing zeros via binary search */
  for (s = CHAR_BIT * (uint)sizeof(UInt); m; s /= 2)
    if ((UInt)(m << (s - 1))) {
      m <<= s - 1;
      m <<= 1;
      p += s;
    }
  return p;
}

/* encode block of integers using reversible algorithm */
static uint
_t2(rev_encode_block, Int, DIMS)(bitstream* stream, int minbits, int maxbits, int maxprec, Int* iblock)
{
  int bits = PBITS;
  int prec;
  cache_align_(UInt ublock[BLOCK_SIZE]);
  /* perform decorrelating transform */
  _t2(rev_fwd_xform, Int, DIMS)(iblock);
  /* reorder signed coefficients and convert to unsigned integer */
  _t1(fwd_order, Int)(ublock, iblock, PERM, BLOCK_SIZE);
  /* determine and encode number of significant bits */
  prec = _t1(rev_precision, UInt)(ublock, BLOCK_SIZE);
  prec = MIN(prec, maxprec);
  prec = MAX(prec, 1);
  stream_write_bits(stream, prec - 1, PBITS);
  /* encode integer coefficients */
  if (BLOCK_SIZE <= 64)
    bits += _t1(encode_ints, UInt)(stream, maxbits - bits, prec, ublock, BLOCK_SIZE);
  else
    bits += _t1(encode_many_ints, UInt)(stream, maxbits - bits, prec, ublock, BLOCK_SIZE);
  /* write at least minbits bits by padding with zeros */
  if (bits < minbits) {
    stream_pad(stream, minbits - bits);
    bits = minbits;
  }
  return bits;
}
