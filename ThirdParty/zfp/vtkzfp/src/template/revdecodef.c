#include <string.h>

/* private functions ------------------------------------------------------- */

/* reinterpret two's complement integers as floating values */
static void
_t1(rev_inv_reinterpret, Scalar)(Int* iblock, Scalar* fblock, uint n)
{
  /* convert two's complement integers to sign-magnitude integers */
  uint i;
  for (i = 0; i < n; i++) {
    Int x = iblock[i];
    if (x < 0)
      iblock[i] = (Int)((UInt)x ^ TCMASK);
  }
  /* reinterpret sign-magnitude integers as floating values */
  memcpy(fblock, iblock, n * sizeof(*fblock));
}

/* decode contiguous floating-point block using reversible algorithm */
static uint
_t2(rev_decode_block, Scalar, DIMS)(zfp_stream* zfp, Scalar* fblock)
{
  uint bits = 0;
  cache_align_(Int iblock[BLOCK_SIZE]);
  /* test whether block is all-zero */
  bits++;
  if (stream_read_bit(zfp->stream)) {
    /* non-zero block; test whether to use block-floating-point transform */
    bits++;
    if (stream_read_bit(zfp->stream)) {
      /* decode integer block */
      bits += _t2(rev_decode_block, Int, DIMS)(zfp->stream, zfp->minbits - bits, zfp->maxbits - bits, iblock);
      /* reinterpret integers as floating values */
      _t1(rev_inv_reinterpret, Scalar)(iblock, fblock, BLOCK_SIZE);
    }
    else {
      /* decode common exponent */
      bits += EBITS;
      int emax = (int)stream_read_bits(zfp->stream, EBITS) - EBIAS;
      /* decode integer block */
      bits += _t2(rev_decode_block, Int, DIMS)(zfp->stream, zfp->minbits - bits, zfp->maxbits - bits, iblock);
      /* perform inverse block-floating-point transform */
      _t1(rev_inv_cast, Scalar)(iblock, fblock, BLOCK_SIZE, emax);
    }
  }
  else {
    /* all-zero block; set all values to zero */
    uint i;
    for (i = 0; i < BLOCK_SIZE; i++)
      *fblock++ = 0;
    if (zfp->minbits > bits) {
      stream_skip(zfp->stream, zfp->minbits - bits);
      bits = zfp->minbits;
    }
  }
  return bits;
}
