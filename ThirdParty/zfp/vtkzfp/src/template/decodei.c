/* public functions -------------------------------------------------------- */

/* decode contiguous integer block */
uint
_t2(zfp_decode_block, Int, DIMS)(zfp_stream* zfp, Int* iblock)
{
  return _t2(decode_block, Int, DIMS)(zfp->stream, zfp->minbits, zfp->maxbits, zfp->maxprec, iblock);
}
