/* public functions -------------------------------------------------------- */

/* encode contiguous integer block */
uint
_t2(zfp_encode_block, Int, DIMS)(zfp_stream* zfp, const Int* iblock)
{
  cache_align_(Int block[BLOCK_SIZE]);
  uint i;
  /* copy block */
  for (i = 0; i < BLOCK_SIZE; i++)
    block[i] = iblock[i];
  return _t2(encode_block, Int, DIMS)(zfp->stream, zfp->minbits, zfp->maxbits, zfp->maxprec, block);
}
