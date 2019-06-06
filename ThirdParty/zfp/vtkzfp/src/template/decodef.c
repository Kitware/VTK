static uint _t2(rev_decode_block, Scalar, DIMS)(zfp_stream* zfp, Scalar* fblock);

/* private functions ------------------------------------------------------- */

/* decode contiguous floating-point block using lossy algorithm */
static uint
_t2(decode_block, Scalar, DIMS)(zfp_stream* zfp, Scalar* fblock)
{
  uint bits = 1;
  /* test if block has nonzero values */
  if (stream_read_bit(zfp->stream)) {
    cache_align_(Int iblock[BLOCK_SIZE]);
    /* decode common exponent */
    bits += EBITS;
    int emax = (int)stream_read_bits(zfp->stream, EBITS) - EBIAS;
    int maxprec = precision(emax, zfp->maxprec, zfp->minexp, DIMS);
    /* decode integer block */
    bits += _t2(decode_block, Int, DIMS)(zfp->stream, zfp->minbits - bits, zfp->maxbits - bits, maxprec, iblock);
    /* perform inverse block-floating-point transform */
    _t1(inv_cast, Scalar)(iblock, fblock, BLOCK_SIZE, emax);
  }
  else {
    /* set all values to zero */
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

/* public functions -------------------------------------------------------- */

/* decode contiguous floating-point block */
uint
_t2(zfp_decode_block, Scalar, DIMS)(zfp_stream* zfp, Scalar* fblock)
{
  return REVERSIBLE(zfp) ? _t2(rev_decode_block, Scalar, DIMS)(zfp, fblock) : _t2(decode_block, Scalar, DIMS)(zfp, fblock);
}
