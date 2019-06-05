#include <string.h>

/* private functions ------------------------------------------------------- */

/* test if block-floating-point encoding is reversible */
static int
_t1(rev_fwd_reversible, Scalar)(const Int* iblock, const Scalar* fblock, uint n, int emax)
{
  /* reconstruct block */
  cache_align_(Scalar gblock[BLOCK_SIZE]);
  _t1(rev_inv_cast, Scalar)(iblock, gblock, n, emax);
  /* perform bit-wise comparison */
  return !memcmp(fblock, gblock, n * sizeof(*fblock));
}

/* forward block-floating-point transform to signed integers */
static void
_t1(rev_fwd_cast, Scalar)(Int* iblock, const Scalar* fblock, uint n, int emax)
{
  /* test for all-zero block, which needs special treatment */
  if (emax != -EBIAS)
    _t1(fwd_cast, Scalar)(iblock, fblock, BLOCK_SIZE, emax);
  else
    while (n--)
      *iblock++ = 0;
}

/* reinterpret floating values as two's complement integers */
static void
_t1(rev_fwd_reinterpret, Scalar)(Int* iblock, const Scalar* fblock, uint n)
{
  /* reinterpret floating values as sign-magnitude integers */
  memcpy(iblock, fblock, n * sizeof(*iblock));
  /* convert sign-magnitude integers to two's complement integers */
  while (n--) {
    Int x = *iblock;
    if (x < 0)
      *iblock = (Int)((UInt)x ^ TCMASK);
    iblock++;
  }
}

/* encode contiguous floating-point block using reversible algorithm */
static uint
_t2(rev_encode_block, Scalar, DIMS)(zfp_stream* zfp, const Scalar* fblock)
{
  uint bits = 0;
  cache_align_(Int iblock[BLOCK_SIZE]);
  /* compute maximum exponent */
  int emax = _t1(exponent_block, Scalar)(fblock, BLOCK_SIZE);
  /* perform forward block-floating-point transform */
  _t1(rev_fwd_cast, Scalar)(iblock, fblock, BLOCK_SIZE, emax);
  /* test if block-floating-point transform is reversible */
  if (_t1(rev_fwd_reversible, Scalar)(iblock, fblock, BLOCK_SIZE, emax)) {
    /* transform is reversible; test if block has any non-zeros */
    uint e = emax + EBIAS;
    if (e) {
      /* encode common exponent */
      bits += 2;
      stream_write_bits(zfp->stream, 1, 2);
      bits += EBITS;
      stream_write_bits(zfp->stream, e, EBITS);
    }
    else {
      /* emit single bit for all-zero block */
      bits++;
      stream_write_bit(zfp->stream, 0);
      return bits;
    }
  }
  else {
    /* transform is irreversible; reinterpret floating values as integers */
    _t1(rev_fwd_reinterpret, Scalar)(iblock, fblock, BLOCK_SIZE);
    bits++;
    stream_write_bits(zfp->stream, 3, 2);
  }
  /* losslessly encode integers */
  bits += _t2(rev_encode_block, Int, DIMS)(zfp->stream, zfp->minbits - bits, zfp->maxbits - bits, zfp->maxprec, iblock);
  return bits;
}
